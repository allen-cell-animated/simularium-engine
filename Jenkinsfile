pipeline {
    options {
        disableConcurrentBuilds()
        timeout(time: 2, unit: 'HOURS')
    }
    agent { label "docker" }
    stages {
        stage ("notify-start") {
            steps {
                this.notifyBB("INPROGRESS")
            }
        }
        stage ("build") {
            steps {
                sh "/usr/bin/git fetch --tags"
                sh "./build.sh -r"
            }
        }
        stage ("git-tag-and-push") {
            when {
                branch "main"
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
                branch "main"
            }
            steps {
                sh "./build.sh -p ${tag}"
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
