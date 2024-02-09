pipeline {
    agent any
   
    // environment {
       // DOCKER_IMAGE_NAME = 'my_ubuntu_image'
       // DOCKERFILE_PATH = 'docker pull mavrikraj/my_ubuntu_image'
       // GITHUB_REPO_URL = 'https://github.com/Rajendra-46/CommissioningApi.git'
  //  }
    
    stages {
        stage('Checkout') {
            
            steps {
                  checkout scmGit(branches: [[name: 'main']], extensions: [], userRemoteConfigs: [[credentialsId: 'Commission_jenkins', url: 'https://github.com/Rajendra-46/CommissioningApi']])
            }
        }
        
        stage('Compile code') {
           steps{
		//sh "cppcheck *.cpp --enable=all"
                sh "qmake CommissioningTool.pro"
		sh "make"
           }
        }
    }
}
