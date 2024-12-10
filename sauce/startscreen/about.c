#include "about.h"
#include <PalmOS.h>
#include "../resources.h"

Boolean about_buttonHandler(EventPtr eventptr, int returnFormId) {
    if (FrmGetActiveFormID() != ABOUT_FORM) {
        return false;
    }
    switch (eventptr->eType) {
    case ctlSelectEvent:
        switch (eventptr->data.ctlSelect.controlID) {
        case ABOUT_FORM_BUTTON_ACKNOWLEDGEMENTS:
            FrmCustomAlert(ABOUT_ALERT_ACKNOWLEDGEMENTS, NULL, NULL, NULL);
            return true;
        default:
            FrmReturnToForm(returnFormId);
            return true;
        }
    default:
        return false;
    }
}

Boolean about_open() {
    FormType *frmP = FrmInitForm(ABOUT_FORM);
    FrmSetActiveForm(frmP);
    FrmDrawForm(frmP);
}