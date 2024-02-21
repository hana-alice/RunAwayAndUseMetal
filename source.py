import os


# cd dependencies/shaderc
# python ./utils/git-sync-deps
curDir = os.getcwd()
os.chdir(curDir + "/dependencies/shaderc")
os.system("python ./utils/git-sync-deps")