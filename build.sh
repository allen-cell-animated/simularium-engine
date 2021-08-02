#!/bin/bash -ex

function get_aws_profile_name {
    /usr/bin/jq -e --raw-output '.aws_profile_name' build.json
}

function get_branch_name {
    # BRANCH_NAME contains the branch on Jenkins, unset elsewhere:
    if [[ ${BRANCH_NAME+x} != 'x' ]] ; then
        # Easier to parse current branch than "git branch -l":
        /usr/bin/git symbolic-ref --short HEAD
    else
        echo ${BRANCH_NAME}
    fi
}

function get_image_name {
    /usr/bin/jq -e --raw-output '.image_name' build.json
}

function get_stage_registry {
    /usr/bin/jq -e --raw-output '.stage_registry' build.json
}

function next_image_tag {
    # Echoes the next Docker image tag to use by comparing
    # "base_aics_version" from build.json and the latest tag in git
    # that strictly matches semver.  If "base_aics_version" is greater
    # than the latest git tag (or there is no latest git tag) then it
    # is returned.  Otherwise the git tag patch version is incremented
    # and returned.

    branch_name=$(get_branch_name)

    if [[ $branch_name == 'main' ]] ; then
        base_aics_version=$(/usr/bin/jq -e --raw-output '.base_aics_version' build.json)
        latest_tag=$(/usr/bin/git tag -l --sort version:refname | /bin/egrep '^[0-9]+\.[0-9]+\.[0-9]+$' | tail -1)
        if [[ $latest_tag == '' ]] ; then
            tag=$base_aics_version
        else
            version_compare=$(semver compare $base_aics_version $latest_tag)
            if [[ $version_compare == 1 ]] ; then
                tag=$base_aics_version
            else
                tag=$(semver bump patch $latest_tag)
            fi
        fi
    else
        # Create docker image tag from git branch; filter characters not
        # allowed in docker tags from the branch name, replace with underscore:
        tag=$(echo $branch_name | sed 's/[^A-z0-9._-]/_/g')
    fi
    echo $tag
}

function promote {
    if (( $# != 1 )); then
        echo 'promote function must be passed a single git tag to promote'
        exit 1
    fi
    git_tag_to_promote=$1
    image_name=$(get_image_name)
    stage_registry=$(get_stage_registry)
    image=${stage_registry}/${image_name}:${git_tag_to_promote}
    aws_profile_name=$(get_aws_profile_name)
    set +e
    /usr/bin/docker pull ${image}
    [[ $? != 0 ]] && echo "Could not pull ${image} - continuing." && continue
    set -e
    for promo_reg in $(/usr/bin/jq -e --raw-output '.push_promote_registries[]' build.json); do
        promo_image=${promo_reg}/${image_name}:${git_tag_to_promote}
        /usr/bin/docker tag ${image} ${promo_image}
        AWS_PROFILE=${aws_profile_name} /usr/bin/docker push ${promo_image}
    done
    for art_reg in $(/usr/bin/jq -e --raw-output '.artifactory_promote_registries[]' build.json); do
        # Read API key - should be assigned to $art_api_key in this file by AICS convention:
        . ~/.artifactory

        promote_payload="{ \"targetRepo\": \"${art_reg}\",
                           \"dockerRepository\": \"$(get_image_name)\",
                           \"tag\": \"${git_tag_to_promote}\" }"

        /usr/bin/curl -i \
                      -u cipublisher:${art_api_key} \
                      -X POST \
                      $(/usr/bin/jq -e --raw-output '.artifactory_stage_promote_url' build.json) \
                      -H "Content-Type: application/json" \
                      -d "$promote_payload"
    done
}

set -u

push_to_remote=0
while getopts 'p:rt' flag; do
    case $flag in
        p) promote $OPTARG && exit 0 ;;
        r) push_to_remote=1 ;;
        t) next_image_tag && exit 0 ;;
    esac
done

image_name=$(get_image_name)
image_tag=$(next_image_tag)
image_name_tag="${image_name}:${image_tag}"
/usr/bin/docker build --pull -t ${image_name_tag} .
if [[ $push_to_remote == 1 ]] ; then
    stage_registry=$(get_stage_registry)
    /usr/bin/docker tag ${image_name_tag} ${stage_registry}/${image_name_tag}
    /usr/bin/docker push ${stage_registry}/${image_name_tag}
    if [[ $(get_branch_name) == 'main' ]] ; then
        latest_snap_tag="${image_name}:latest-snapshot"
        /usr/bin/docker tag ${image_name_tag} ${stage_registry}/${latest_snap_tag}
        /usr/bin/docker push ${stage_registry}/${latest_snap_tag}
    fi
fi
