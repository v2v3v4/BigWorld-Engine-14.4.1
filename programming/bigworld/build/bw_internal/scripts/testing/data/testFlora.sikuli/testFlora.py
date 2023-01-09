TIMEOUT = 60
modeleditor_path = "\\game\\bin\\tools\\modeleditor\\win64\\modeleditor.exe -noConversion"
FOCOUSE_ON_PROGRAM = "focuseOnProgram.png"

def ClickAndWait(button_img, img_res):
    #Click on button_img and wait for img_res
    click(button_img)
    wait(img_res,TIMEOUT)
    
def StartModelEditor(modeleditor_path):
    openApp(modeleditor_path)
    PROGRAN_OPENED = "programOpened.png" 
    #wait for model editor to start
    wait(PROGRAN_OPENED,2*TIMEOUT)
    click(FOCOUSE_ON_PROGRAM)
    
def TestDisplaySettings():
    #Click on display settings
    DISPLAY_BUTTON = "displayButton.png"
    DISPLAY_WINDOW = "displayWindow.png"
    if not exists(DISPLAY_WINDOW):
        ClickAndWait(DISPLAY_BUTTON, DISPLAY_WINDOW)
        click(FOCOUSE_ON_PROGRAM)

    #change Background to terrain
    print(">>>>>>>change Background to terrain<<<<<<<<<<<")
    
    CHANGE_BACKGROUND_FLOOR_BUTTON = Pattern("changeBackgroundFloorButton-1.png").similar(0.93)
    CHANGE_BACKGROUND_NO_BACKGROUND_BUTTON = Pattern("changeBackgroundFloorButton.png").similar(0.91)
    CHANGE_BACKGROUND_TO_TERRAIN = Pattern("useTerrain.png").similar(0.97)
    TERRAIN_BUTTON = Pattern("terrainButton.png").similar(0.80)
    if exists(CHANGE_BACKGROUND_FLOOR_BUTTON):
        print("exists CHANGE_BACKGROUND_FLOOR_BUTTON")
        ClickAndWait(CHANGE_BACKGROUND_FLOOR_BUTTON, CHANGE_BACKGROUND_TO_TERRAIN)
        ClickAndWait(CHANGE_BACKGROUND_TO_TERRAIN, TERRAIN_BUTTON)
    elif exists(CHANGE_BACKGROUND_NO_BACKGROUND_BUTTON):
        print("exists CHANGE_BACKGROUND_NO_BACKGROUND_BUTTON")
        ClickAndWait(CHANGE_BACKGROUND_NO_BACKGROUND_BUTTON, CHANGE_BACKGROUND_TO_TERRAIN)
        ClickAndWait(CHANGE_BACKGROUND_TO_TERRAIN, TERRAIN_BUTTON)
    click(FOCOUSE_ON_PROGRAM)
    
    #disable Flora    
    print(">>>>>>>disable Flora<<<<<<<<<<<")
    
    DISABLE_FLORA_BUTTON = Pattern("disableFloraButton.png").similar(0.80)
    LOW_FLORA_BUTTON = Pattern("lowFloraButton.png").similar(0.84)
    MEDUIM_FLORA_BUTTON = Pattern("meduimFloraButton.png").similar(0.83)
    HIGH_FLORA_BUTTON = Pattern("veryFloraButton.png").similar(0.83)
    VERY_HIGH_FLORA_BUTTON = Pattern("veryHighFloraList.png").similar(0.80)
    
    DISABLE_FLORA_LIST = Pattern("disableFlora-2.png").similar(0.98)
    LOW_FLORA_LIST = "lowFloraList.png"
    MEDUIM_FLORA_LIST = "meduimFloraList.png"
    HIGH_FLORA_LIST = "highFloraList.png"
    VERY_HIGH_FLORA_LIST = "veryHighFloraList-1.png"
    
    if exists(LOW_FLORA_BUTTON):
        print("exists LOW_FLORA_BUTTON")
        ClickAndWait(LOW_FLORA_BUTTON, DISABLE_FLORA_LIST)
        ClickAndWait(DISABLE_FLORA_LIST, DISABLE_FLORA_BUTTON)
    elif exists(MEDUIM_FLORA_BUTTON):
        print("exists MEDUIM_FLORA_BUTTON")
        ClickAndWait(MEDUIM_FLORA_BUTTON, DISABLE_FLORA_LIST)
        ClickAndWait(DISABLE_FLORA_LIST, DISABLE_FLORA_BUTTON)
    elif exists(HIGH_FLORA_BUTTON):
        print("exists HIGH_FLORA_BUTTON")
        ClickAndWait(LOW_FLORA_BUTTON, DISABLE_FLORA_LIST)
        ClickAndWait(DISABLE_FLORA_LIST, DISABLE_FLORA_BUTTON)
    elif exists(VERY_HIGH_FLORA_BUTTON):
        print("exists VERY_HIGH_FLORA_BUTTON")
        ClickAndWait(LOW_FLORA_BUTTON, DISABLE_FLORA_LIST)
        ClickAndWait(DISABLE_FLORA_LIST, DISABLE_FLORA_BUTTON)
    click(FOCOUSE_ON_PROGRAM)
   
    DISABLE_FLORA = "noFlora.png" 
    LOW_FLORA = "lowFlora.png"
    MEDUIM_FLORA = "meduimFlora.png"
    HIGH_FLORA = "highFlora.png"
    VERY_HIGH_FLORA = "veryHighFlora.png"

    print(">>>>>>> test disable<<<<<<<<<<<")
    #test Disable
    wait(DISABLE_FLORA,TIMEOUT)
    click(FOCOUSE_ON_PROGRAM)

    print(">>>>>>> test LOW_FLORA<<<<<<<<<<<")
    #test LOW_FLORA
    ClickAndWait(DISABLE_FLORA_BUTTON, LOW_FLORA_LIST)
    ClickAndWait(LOW_FLORA_LIST, LOW_FLORA)
    click(FOCOUSE_ON_PROGRAM)
    
    print(">>>>>>> test MEDUIM_FLORA<<<<<<<<<<<")
    #test LOW_FLORA
    ClickAndWait(LOW_FLORA_BUTTON, MEDUIM_FLORA_LIST)
    ClickAndWait(MEDUIM_FLORA_LIST, MEDUIM_FLORA)
    click(FOCOUSE_ON_PROGRAM)
    
    print(">>>>>>> test HIGH_FLORA<<<<<<<<<<<")
    #test HIGH_FLORA
    ClickAndWait(MEDUIM_FLORA_BUTTON, HIGH_FLORA_LIST)
    ClickAndWait(HIGH_FLORA_LIST, HIGH_FLORA)
    click(FOCOUSE_ON_PROGRAM)
    
    print(">>>>>>> test VERY_HIGH_FLORA<<<<<<<<<<<")
    #test VERY_HIGH_FLORA
    ClickAndWait(HIGH_FLORA_BUTTON, VERY_HIGH_FLORA_LIST)
    ClickAndWait(VERY_HIGH_FLORA_LIST, VERY_HIGH_FLORA)



# ============== main function ==========
console=False

if len(sys.argv) > 1:
    modeleditor_path = sys.argv[1] + modeleditor_path
else:
    print "ERORR: missing argumants for the Model and Model editor location"
    modeleditor_path = "c:\\bw\\2_current" + modeleditor_path
    #sys.exit(1)

StartModelEditor(modeleditor_path)
sleep(1)
TestDisplaySettings()
print(">>>>>>>FINISH TEST - SUCSSESS<<<<<<<<<<<")

