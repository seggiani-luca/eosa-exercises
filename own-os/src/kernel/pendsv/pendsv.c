#include "../include/pendsv.h"
#include "../include/cmsdk.h"

void pendsv_request() {
	// make PendSV pending
    SCB->ICSR = SCB_ICSR_PENDSVSET;
}
