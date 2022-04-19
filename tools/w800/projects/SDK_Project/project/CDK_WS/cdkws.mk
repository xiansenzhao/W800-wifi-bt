.PHONY: clean All Project_Title Project_Build Project_PostBuild

All: Project_Title Project_Build Project_PostBuild

Project_Title:
	@echo "----------Building project:[ W800_SDK - BuildSet ]----------"

Project_Build:
	@make -r -f W800_SDK.mk -j 6 -C  W800_SDK 

Project_PostBuild:
	@echo Executing Post Build commands ...
	@cd "W800_SDK" && export CDKPath="D:/C-Sky/CDK" CDK_VERSION="V2.10.3" ProjectPath="D:/PROJECT_SDK/wm_sdk_w800_wifi_bt/tools/w800/projects/SDK_Project/project/CDK_WS/W800_SDK/" && ../../../../../../../tools/w800/utilities/cdk_aft_build.sh;../../../../../../../tools/w800/utilities/aft_build_project.sh 
	@echo Done


clean:
	@echo "----------Cleaning project:[ W800_SDK - BuildSet ]----------"

