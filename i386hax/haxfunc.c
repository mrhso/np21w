#include	"compiler.h"
#include	"haxfunc.h"
#include	"haxcore.h"

#if defined(SUPPORT_IA32_HAXM)

// HAXM IOCTL
UINT8 i386haxfunc_getversion(HAX_MODULE_VERSION *version) {

	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hDevice, HAX_IOCTL_VERSION,
                          NULL, 0,
						  version, sizeof(HAX_MODULE_VERSION), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_getcapability(HAX_CAPINFO *cap) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hDevice, HAX_IOCTL_CAPABILITY,
                          NULL, 0,
						  cap, sizeof(HAX_CAPINFO), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_setmemlimit(HAX_SET_MEMLIMIT *memlimit) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hDevice, HAX_IOCTL_SET_MEMLIMIT,
                          memlimit, sizeof(HAX_SET_MEMLIMIT),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_createVM(UINT32 *vm_id) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hDevice, HAX_IOCTL_CREATE_VM,
                          NULL, 0,
						  vm_id, sizeof(UINT32), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}

// HAXM VM IOCTL
UINT8 i386haxfunc_notifyQEMUversion(HAX_QEMU_VERSION *hax_qemu_ver) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVMDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVMDevice, HAX_VM_IOCTL_NOTIFY_QEMU_VERSION,
                          hax_qemu_ver, sizeof(HAX_QEMU_VERSION),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}

UINT8 i386haxfunc_VCPUcreate(UINT32 vcpu_id) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVMDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVMDevice, HAX_VM_IOCTL_VCPU_CREATE,
                          &vcpu_id, sizeof(UINT32),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}

UINT8 i386haxfunc_allocRAM(HAX_ALLOC_RAM_INFO *allocraminfo) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVMDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVMDevice, HAX_VM_IOCTL_ALLOC_RAM,
                          allocraminfo, sizeof(HAX_ALLOC_RAM_INFO),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
		ret = GetLastError();
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_addRAMblock(HAX_RAMBLOCK_INFO *ramblkinfo) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVMDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVMDevice, HAX_VM_IOCTL_ADD_RAMBLOCK,
                          ramblkinfo, sizeof(HAX_RAMBLOCK_INFO),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
		ret = GetLastError();
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_setRAM(HAX_SET_RAM_INFO *setraminfo) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVMDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVMDevice, HAX_VM_IOCTL_SET_RAM,
                          setraminfo, sizeof(HAX_SET_RAM_INFO),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}

// HAXM VCPU IOCTL
UINT8 i386haxfunc_vcpu_run(){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_RUN,
                          NULL, 0,
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_setup_tunnel(HAX_TUNNEL_INFO *info){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_SETUP_TUNNEL,
                          NULL, 0,
						  info, sizeof(HAX_TUNNEL_INFO), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_setMSRs(HAX_MSR_DATA *inbuf, HAX_MSR_DATA *outbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_SET_MSRS,
                          inbuf, sizeof(HAX_MSR_DATA),
						  outbuf, sizeof(HAX_MSR_DATA), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_getMSRs(HAX_MSR_DATA *outbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_GET_MSRS,
                          NULL, 0,
						  outbuf, sizeof(HAX_MSR_DATA), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_setFPU(HAX_FX_LAYOUT *inbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_SET_FPU,
                          inbuf, sizeof(HAX_FX_LAYOUT),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_getFPU(HAX_FX_LAYOUT *outbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_GET_FPU,
                          NULL, 0,
						  outbuf, sizeof(HAX_FX_LAYOUT), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_setREGs(HAX_VCPU_STATE *inbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_SET_REGS,
                          inbuf, sizeof(HAX_VCPU_STATE),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_getREGs(HAX_VCPU_STATE *outbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_GET_REGS,
                          NULL, 0,
						  outbuf, sizeof(HAX_VCPU_STATE), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_interrupt(UINT32 vector){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_INTERRUPT,
                          &vector, sizeof(UINT32),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_kickoff(){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_KICKOFF,
                          NULL, 0,
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_debug(HAX_DEBUG *inbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_IOCTL_VCPU_DEBUG,
                          inbuf, sizeof(HAX_DEBUG),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}

	return SUCCESS;
}
#endif