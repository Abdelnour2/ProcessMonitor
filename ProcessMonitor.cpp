#include <ntddk.h>

void ProcessMonitorUnload(PDRIVER_OBJECT DriverObject);
void ProcessNotifyRoutine(PEPROCESS, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	KdPrint(("Process Monitor Driver - Driver Entry\n"));

	DriverObject->DriverUnload = ProcessMonitorUnload;

	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\device\\ProcessMonitor");
	PDEVICE_OBJECT deviceObject;
	NTSTATUS status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed at IoCreateDevice - Error 0x%X\n", status));

		return status;
	}

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\ProcessMonitor");
	status = IoCreateSymbolicLink(&symLink, &deviceName);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed at IoCreateSymbolicLink - Error 0x%X\n", status));

		IoDeleteDevice(deviceObject);

		return status;
	}

	status = PsSetCreateProcessNotifyRoutineEx(ProcessNotifyRoutine, FALSE);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed at PsSetCreateProcessNotifyRoutineEx - Error 0x%X\n", status));

		IoDeleteSymbolicLink(&symLink);
		IoDeleteDevice(deviceObject);

		return status;
	}

	return STATUS_SUCCESS;
}

void ProcessMonitorUnload(PDRIVER_OBJECT DriverObject) {
	KdPrint(("Process Monitor Driver - Unloaded\n"));

	PsSetCreateProcessNotifyRoutineEx(ProcessNotifyRoutine, TRUE);

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\ProcessMonitor");
	IoDeleteSymbolicLink(&symLink);

	IoDeleteDevice(DriverObject->DeviceObject);

}

void ProcessNotifyRoutine(PEPROCESS, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
	if (CreateInfo) {
		KdPrint(("Process Created: %wZ - Process ID: %p\n", CreateInfo->ImageFileName, ProcessId));
	}
	else {
		KdPrint(("Process Exited: Process ID: %p\n", ProcessId));
	}
}