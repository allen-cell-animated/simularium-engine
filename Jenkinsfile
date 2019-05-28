pipeline {
    options {
        disableConcurrentBuilds()
        timeout(time: 1, unit: 'HOURS')
    }
    agent { label "docker" }
    stages {
        stage ("notify-start") {
            steps {
                this.notifyBB("INPROGRESS")
            }
        }
        stage ("build") {
            when {
                expression { params.promote_artifact == null }
            }
            steps {
                sh "/usr/bin/git fetch --tags"
                sh "./build.sh -r"
            }
        }
        stage ("git-tag-and-push") {
            when {
                expression { params.promote_artifact == null }
                branch "master"
            }
            steps {
                // sh can only return values in a script step:
                script {
                    tag = sh (script: "./build.sh -t",
                              returnStdout: true).trim()
                }
                sh "/usr/bin/git tag ${tag}"
                sh "/usr/bin/git push --tags"
            }
        }
        stage ("promote") {
            when {
                expression { return params.promote_artifact }
            }
            steps {
                sh "./build.sh -p ${params.git_tag}"
            }
        }
    }
    post {
        always {
            this.notifyBB(currentBuild.result)
        }
        cleanup {
            deleteDir()
        }
    }
}

def notifyBB(String state) {
    // on success, result is null
    state = state ?: "SUCCESS"

    if (state == "SUCCESS" || state == "FAILURE") {
        currentBuild.result = state
    }

    notifyBitbucket commitSha1: "${GIT_COMMIT}",
    credentialsId: 'aea50792-dda8-40e4-a683-79e8c83e72a6',
    disableInprogressNotification: false,
    considerUnstableAsSuccess: true,
    ignoreUnverifiedSSLPeer: false,
    includeBuildNumberInKey: false,
    prependParentProjectKey: false,
    projectKey: 'SW',
    stashServerBaseUrl: 'https://aicsbitbucket.corp.alleninstitute.org'
}
