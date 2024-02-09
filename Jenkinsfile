pipeline {
    agent any
   
    // environment {
       // DOCKER_IMAGE_NAME = 'my_ubuntu_image'
       // DOCKERFILE_PATH = 'docker pull mavrikraj/my_ubuntu_image'
   //  }
    
    stages {
        stage('Checkout') {
            
            steps {
                  checkout scm  // if Repository private then used this -  scmGit(branches: [[name: 'main']], extensions: [], userRemoteConfigs: [[credentialsId: 'Commission_jenkins', url: 'https://github.com/Rajendra-46/CommissioningApi']])
            }
        }
	stage('Clean'){
		steps{
	           // clean the project before build
	           sh "make clean"
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
