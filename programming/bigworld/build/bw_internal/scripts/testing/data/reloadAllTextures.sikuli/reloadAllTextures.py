import shutil
TIMEOUT = 60
modeleditor_path = "\\game\\bin\\tools\\modeleditor\\win64\\modeleditor.exe -noConversion"
dir_path = "\\game\\res\\fantasydemo\\characters\\avatars\\ranger"
FOCOUSE_ON_PROGRAM = "focuseOnProgram.png"

def moveTexture(source_org, destination):
    source = os.listdir(source_org)
    for files in source:
        if files.endswith(".bmp") or files.endswith(".dds") or files.endswith(".tga"):
            shutil.move(source_org + "\\" + files,destination)    
            
def StartModelEditor(modeleditor_path):
    openApp(modeleditor_path)
    PROGRAN_OPENED = "programOpened.png" 
    #wait for model editor to start
    wait(PROGRAN_OPENED,2*TIMEOUT)
    click(FOCOUSE_ON_PROGRAM)

def waitForTextureRanger():
    RANGER_HAND = "rangerHand.png"
    RANGER_BACK = "rangerBack.png"
    RANGER_BODY = "rangerBody.png"
    wait(RANGER_BODY,TIMEOUT)
    wait(RANGER_BACK,TIMEOUT)
    wait(RANGER_HAND,TIMEOUT)

def waitForNOTTextureRanger():
    NO_TEXTURE_RANGER_HAND = "notTextureRangerHand.png"
    NO_TEXTURE_RANGER_BELT = "notTextureRangerBelt.png"
    NO_TEXTURE_RANGER_BODY = "notTextureRangerBody.png"
    wait(NO_TEXTURE_RANGER_BODY,TIMEOUT)
    wait(NO_TEXTURE_RANGER_BELT,TIMEOUT)
    wait(NO_TEXTURE_RANGER_HAND,TIMEOUT)
    
def TestReloadAllTextures():
    waitForTextureRanger()
    print "Move Textures"
    moveTexture(dir_path, dir_path + "\\animations")
    print "Refrush model"
    click(FOCOUSE_ON_PROGRAM)
    type("t", KEY_CTRL)
    click(FOCOUSE_ON_PROGRAM)
    waitForNOTTextureRanger()
    print "Return Textures"
    moveTexture(dir_path + "\\animations", dir_path)
    sleep(1)
    print "Refrush model"
    click(FOCOUSE_ON_PROGRAM)
    type("t", KEY_CTRL)
    sleep(1)
    click(FOCOUSE_ON_PROGRAM)
    type("t", KEY_CTRL)
    click(FOCOUSE_ON_PROGRAM)
    waitForTextureRanger()
    
# ============== main function ==========
console=False


if len(sys.argv) > 1:
    modeleditor_path = sys.argv[1] + modeleditor_path
    dir_path = sys.argv[1] + dir_path
else:
    print "ERORR: missing argumants for the Model and Model editor location"
    modeleditor_path = "c:\\bw\\2_current" + modeleditor_path
    dir_path = "c:\\bw\\2_current" + dir_path
    #sys.exit(1)

StartModelEditor(modeleditor_path)
sleep(1)
TestReloadAllTextures()
print(">>>>>>>FINISH TEST<<<<<<<<<<<")

