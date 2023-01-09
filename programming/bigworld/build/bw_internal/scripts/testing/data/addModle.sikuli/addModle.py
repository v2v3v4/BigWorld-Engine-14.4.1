TIMEOUT = 60
modeleditor_path = "\\game\\bin\\tools\\modeleditor\\win64\\modeleditor.exe -noConversion"
model_path = "\\game\\res\\fantasydemo\\characters\\avatars\\ranger\\ranger_head.model"
FOCOUSE_ON_PROGRAM = "focuseOnProgram.png"

def ClickAndWait(button_img, img_res):
    #Click on button_img and wait for img_res
    click(button_img)
    wait(img_res,TIMEOUT)
    
def StartModelEditor(modeleditor_path):
    openApp(modeleditor_path)
    #Open model editor and wait for it to start   
    PROGRAN_OPENED = "programOpened.png" 
    wait(PROGRAN_OPENED,2*TIMEOUT)
    click(FOCOUSE_ON_PROGRAM)

def TestAddModel():
    RANGER_BODY = "rangerBody.png"
    RANGER_FULL_BODY = "fullBody.png"
    HEADLESS_RANGER = "headlessRanger.png"
    FILE_BUTTON = "fileButton.png"
    MENU_LIST = "menuList.png"
    ADD_MODEL_BUTTON = "addModel.png"
    REMOVE_ADDED_MODEL_BUTTON = "removeAddedModels.png"
    OPEN_WINDOW = Pattern("openWindow.png").targetOffset(-31,15)

    wait(RANGER_BODY,TIMEOUT)
    #add head
    ClickAndWait(FILE_BUTTON, MENU_LIST)
    ClickAndWait(ADD_MODEL_BUTTON, OPEN_WINDOW)
    type(model_path + Key.ENTER)
    click(FOCOUSE_ON_PROGRAM)
    wait(RANGER_FULL_BODY,TIMEOUT)
    #remove head
    ClickAndWait(FILE_BUTTON, MENU_LIST)
    ClickAndWait(REMOVE_ADDED_MODEL_BUTTON, HEADLESS_RANGER)
  

# ============== main function ==========
console=False

if len(sys.argv) > 1:
    modeleditor_path = sys.argv[1] + modeleditor_path
    model_path = sys.argv[1] + model_path
else:
    print "ERORR: missing argumants for the Model and Model editor location"
    modeleditor_path = "c:\\bw\\2_current" + modeleditor_path
    model_path = "c:\\bw\\2_current" + model_path
    #sys.exit(1)

StartModelEditor(modeleditor_path)
sleep(1)
TestAddModel()
print(">>>>>>>FINISH TEST - SUCSSESS<<<<<<<<<<<")

