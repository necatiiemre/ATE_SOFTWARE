/**
 * @copyright (c) Copyright 2024 by TUBITAK BILGEM
 * All rights reserved.
 * This program is the property of TUBITAK BILGEM and can only be used and
 * copied with the prior written authorization of TUBITAK BILGEM. Any whole
 * or partial copy of this program in either its original from or in a
 * modified form must mention this copyright and its owner.
 *
 * @file gzisTypes.h
 * @brief Defines GzIS types.
 */

#ifndef _TARGET_INC_GZIS_GZISTYPES_H_
#define _TARGET_INC_GZIS_GZISTYPES_H_

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*---------------------------------------------------------------------------*/
/*          STRUCTURES / TYPES / ENUMERATIONS                                */
/*---------------------------------------------------------------------------*/

/**
 * @brief A data type which can store false or true.
 */
typedef BOOL bool_gt;

/**
 * @brief A null pointer constant with the value 0 and #ADDR type.
 */
#ifndef NULL
#define NULL ((addr_gt)0x0UL)
#endif

/**
 * @brief A definiton used to control a function defined weak.
 */
#ifndef NULL_FUNC
#define NULL_FUNC (void*)0
#endif

/**
 * @brief An unsigned char type that is 1 byte in size.
 */
typedef unsigned char uint1_gt;

/**
 * @brief An unsigned integer type that is 2 byte in size.
 */
typedef unsigned short uint2_gt;

/**
 * @brief An unsigned integer type that is 4 byte in size.
 */
typedef unsigned int uint4_gt;

/**
 * @brief An unsigned integer type that is 8 byte in size.
 */
typedef unsigned long long uint8_gt;

/**
 * @brief A char type that is 1 byte in size.
 */
typedef char byte_gt;

/**
 * @brief A signed char type that is 1 byte in size.
 */
typedef signed char int1_gt;

/**
 * @brief A signed integer type that is 2 byte in size.
 */
typedef signed short int2_gt;

/**
 * @brief A signed integer type that is 4 byte in size.
 */
typedef signed int int4_gt;

/**
 * @brief A signed integer type that is 8 byte in size.
 */
typedef signed long long int8_gt;

/**
 * @brief A unsigned type that is used to define architecture-independent
 * parameters.
 * @remarks The size of the type long is equal to the word size of the CPU
 * architecture.
 */
typedef unsigned long uval_gt;

/**
 * @brief A signed type that is used to define architecture-independent
 * parameters.
 * @remarks The size of the type long is equal to the word size of the CPU
 * architecture.
 */
typedef long val_gt;

/**
 * @brief A pointer type that is used to define architecture-independent
 * address pointer.
 */
typedef void * addr_gt;

/**
 * @brief A float type that is 4 byte in size.
 */
typedef float float4_gt;

/**
 * @brief A float type that is 8 byte in size.
 */
typedef double float8_gt;

/**
 * @brief Return values for GzIS functions.
 *
 * @warning Enumeration must start with -1.
 * @warning After first enumeration; ERR_NO_ERROR, ERR_NO_ACTION,
 * ERR_NOT_AVAILABLE, ERR_INVALID_PARAMETER, ERR_INVALID_CONFIG,
 * ERR_INVALID_MODE, and ERR_TIMEOUT should not be touched, even their orders!
 */
typedef enum eError {
    /**
     * @brief Used in undefined cases.
     */
    ERR_NOT_INITIALIZED = -1,

    /**
     * @brief No error returned.
     */
    ERR_NO_ERROR,

    /**
     * @brief For the request no action is taken.
     */
    ERR_NO_ACTION,

    /**
     * @brief The requested action is not supported.
     */
    ERR_NOT_AVAILABLE,

    /**
     * @brief One or more of the function input parameters are invalid.
     */
    ERR_INVALID_PARAMETER,

    /**
     * @brief Request is not valid for the configuration.
     */
    ERR_INVALID_CONFIG,

    /**
     * @brief Action is invalid for active mode.
     */
    ERR_INVALID_MODE,

    /**
     * @brief Waiting period is already expired.
     */
    ERR_TIMEOUT,

    /**
     * @brief Handler is invalid.
     */
    ERR_INVALID_HANDLER,

    /**
     * @brief Not enough memory.
     */
    ERR_NO_MEMORY,

    /**
     * @brief Defined limit is exceeded.
     */
    ERR_LIMIT_EXCEEDED,

    /**
     * @brief Overall system limit is exceeded.
     */
    ERR_SYSTEM_LIMIT_EXCEEDED,

    /**
     * @brief Thread is trying to release that it does not own.
     */
    ERR_ITEM_NOT_OWNED,

    /**
     * @brief Resource that is tried to be owned has already taken by the same
     * thread beforehand.
     */
    ERR_ITEM_ALREADY_OWNED,

    /**
     * @brief Defined absolute wait period is already expired.
     */
    ERR_ALREADY_TIMEOUT,

    /**
     * @brief No thread exists for the requested job.
     */
    ERR_NO_THREAD_FOUND,

    /**
     * @brief Requested resource desired to be created, already created.
     */
    ERR_ALREADY_IN_USE,

    /**
     * @brief Has no privilege to execute requested action.
     */
    ERR_NO_PRIVILEGE,

    /**
     * @brief Requested action is evaluated as it might cause deadlock.
     */
    ERR_DEADLOCK,

    /**
     * @brief Given name too long.
     */
    ERR_NAME_TOO_LONG,

    /**
     * @brief Path is invalid.
     */
    ERR_INVALID_PATH,

    /**
     * @brief Address is not aligned to the page size.
     */
    ERR_NOT_PAGE_ALIGNED,

    /**
     * @brief Page is unmapped.
     */
    ERR_UNMAPPED_PAGE,

    /**
     * @brief Operation was interrupted by the signal.
     */
    ERR_SIGNAL_INTERRUPT,

    /**
     * @brief The value of parameter is out of range.
     */
    ERR_OUT_OF_RANGE,

    /**
     * @brief Access type is not valid.
     */
    ERR_INVALID_ACCESS_TYPE,

    /**
     * @brief Address value is invalid.
     */
    ERR_INVALID_ADDRESS,

    /**
     * @brief Requested resource is not found.
     */
    ERR_NOT_FOUND,

    /**
     * @brief Device not found.
     */
    ERR_NO_DEVICE,

    /**
     * @brief An error occurred during memory allocation for device.
     */
    ERR_DEV_ALLOC,

    /**
     * @brief Device was not initialized by the system.
     */
    ERR_DEV_NOT_INITIALIZED,

    /**
     * @brief Device could not be opened by the driver.
     */
    ERR_DEV_OPEN,

    /**
     * @brief Device could not be closed by the driver.
     */
    ERR_DEV_CLOSE,

    /**
     * @brief Device read error has occurred.
     */
    ERR_DEV_READ,

    /**
     * @brief Device write error has occurred.
     */
    ERR_DEV_WRITE,

    /**
     * @brief The device control function of the driver has failed.
     */
    ERR_DEV_IOCTL,

    /**
     * @brief Event is unknown so no action is taken.
     */
    ERR_EVENT_UNKNOWN,

    /**
     * @brief The event was recognized and handled.
     */
    ERR_EVENT_HANDLED,

    /**
     * @brief Value is not supported.
     */
    ERR_VALUE_NOT_SUPPORTED,

    /**
     * @brief File system could not read from the sector.
     */
    ERR_FS_SECTOR_READ_ERROR,

    /**
     * @brief File system could not write to the sector.
     */
    ERR_FS_SECTOR_WRITE_ERROR,

    /**
     * @brief File system type mismatch.
     */
    ERR_FS_TYPE_NOT_MATCH,

    /**
     * @brief File system is not initialized.
     */
    ERR_FS_NOT_INITIALIZED,

    /**
    * @brief Invalid file system type is detected.
    */
    ERR_FS_TYPE_NOT_VALID,

    /**
     * @brief File system label does not match.
     */
    ERR_FS_LABEL_NOT_MATCH,

    /**
     * @brief File system boot sector is not valid.
     */
    ERR_FS_BOOT_SECTOR_NOT_VALID,

    /**
     * @brief Read error from the boot sector.
     */
    ERR_FS_BOOT_SECTOR_READ_ERROR,

    /**
     * @brief Invalid device name.
     */
    ERR_FS_DEVICE_NAME_NOT_VALID,

    /**
     * @brief File could not be opened.
     */
    ERR_FILE_NOT_OPEN,

	/*
	 * @brief The directory operation can not be performed on another type.
	 */
    ERR_NOT_DIRECTORY,

	/*
	 * @brief The file operation can not be performed on another type.
	 */
	ERR_NOT_FILE,

    /**
     * @brief Maximum file count is exceeded.
     */
    ERR_FILE_MAX_OPEN,

	/*
	 * @brief The maximum file size limit is exceeded.
	 */
	ERR_FILE_TOO_LARGE,

    /**
     * @brief File system has only read permission.
     */
    ERR_FS_READ_ONLY,

    /**
     * @brief File system is not empty.
     */
    ERR_FS_NOT_EMPTY,

    /**
     * @brief The file already exists.
     */
	ERR_FILE_EXISTS,

    /**
     * @brief The directory already exists.
     */
	ERR_DIRECTORY_EXIST,

	/*
	 * @brief The directory is not empty.
	 */
	ERR_DIRECTORY_NOT_EMPTY,

    /**
    * @brief Requested directory or file does not exist.
    */
    ERR_FILE_OR_DIR_EXIST,

    /**
     * @brief File system cannot remove the requested file.
     */
    ERR_FS_CAN_NOT_REMOVE,

    /**
     * @brief File system could not create desired file.
     */
    ERR_FS_CAN_NOT_CREATE,

    /**
     * @brief Connection error occurred.
     */
    ERR_CONNECTION_ERROR,

    /**
     * @brief Connection cannot be closed.
     */
    ERR_CONNECTION_CLOSED,

    /**
     * @brief Route error occurred.
     */
    ERR_ROUTE_ERROR,

    /**
     * @brief No support for the request.
     */
    ERR_NO_SUPPORT,

    /**
     * @brief Address family is not supported.
     */
    ERR_ADDRESS_FAMILY_NOT_SUPPORT,

    /**
     * @brief No such protocol exists.
     */
    ERR_NO_PROTOCOL,

    /**
     * @brief Connection is reset.
     */
    ERR_CONNECTION_RST,

    /**
     * @brief Connection is aborted.
     */
    ERR_CONNECTION_ABRT,

    /**
     * @brief Access failure to the file system.
     */
    ERR_FS_ACCESS_FAILED,

    /**
     * @brief File descriptor is PIPE or FIFO.
     */
    ERR_FD_PIPE_OR_FIFO,

    /**
     * @brief File is already locked.
     */
    ERR_FILE_ALREADY_LOCKED,

    /**
     * @brief Bad file descriptor.
     */
    ERR_BAD_FILE_DESC,

    /**
     * @brief \Message size is invalid.
     */
    ERR_INVALID_MSG_SIZE,

    /**
     * @brief The message is corrupted.
     */
    ERR_BAD_MSG,

    /**
     * @brief Thread could not wait if O_NONBLOCK flag set.
     */
    ERR_THREAD_WOULD_BLOCK,

    /**
     * @brief Request is executed in invalid mode.
     */
    ERR_SOCKET_PIPE,

    /**
     * @brief Request is tried to be executed one sided pipe.
     */
    ERR_PIPE_ERROR,

    /**
     * @brief Priority ceiling violation is occurred.
     */
    ERR_PC_PROTOCOL_CEILING_VIOLATION,

    /**
     * @brief PIP error occurred.
     */
    ERR_PI_PROTOCOL_ERROR,

    /**
     * @brief PIP limit error.
     */
    ERR_PI_PROTOCOL_LIMIT_ERROR,

    /**
     * @brief Agent found a corrupted pack.
     */
    ERR_AGENT_CORRUPTED_PACK,

    /**
     * @brief Agent found checksum error.
     */
    ERR_AGENT_DATA_CHECKSUM,

    /**
     * @brief Requested process/thread is in progress.
     */
    ERR_IN_PROGRESS,

    /**
     * @brief Socket descriptor is invalid.
     */
    ERR_INVALID_SOCKET_DESCRIPTOR,

    /**
     * @brief No connection.
     */
    ERR_NO_CONNECTION,

    /**
     * @brief Socket type is invalid
     */
    ERR_INVALID_SOCKET_TYPE,

    /**
     * @brief Address is not available.
     */
    ERR_ADDR_NOT_AVAILABLE,

    /**
     * @brief No destination address.
     */
    ERR_NO_DESTINATION_ADDRESS,

    /**
     * @brief Option is not valid.
     */
    ERR_INVALID_OPTION,

    /**
     * @brief Exit request is sent to the thread, thread shall exit.
     */
    ERR_THREAD_SHALL_EXIT,

    /**
     * @brief Unsupported system call.
     */
    ERR_SYSCALL_NOT_SUPPORTED,

    /**
     * @brief Invalid file type.
     */
    ERR_INVALID_FILE_TYPE,

    /**
     * @brief In exec system call image reading is unsuccessful.
     */
    ERR_EXEC_READ,

    /**
     * @brief Thread cancelled.
     */
    ERR_THREAD_CANCELLED,

    /**
    * @brief Request could not be performed.
    */
    ERR_FAILURE,

    /**
     * @brief Item already removed.
     */
    ERR_ITEM_ALREADY_REMOVED,

    /**
     * @brief The mutex is a robust mutex and the process containing the
     * previous owning thread terminated while holding the mutex lock.
     * The mutex lock shall be acquired by the calling thread and it is up
     * to the new owner to make the state consistent.
     */
    GZIS_EOWNERDEAD,

    /**
     * @brief The state protected by the mutex is not recoverable.
     */
    GZIS_ENOTRECOVERABLE,
	/**
	 * @brief There is no free space on the media.
	 */
	ERR_NO_SPACE,
	 /**
	 * @brief Device or resource busy.
	 */
	ERR_BUSY,

    /**
     * @brief Currently carried out operation is cancelled.
     */
    ERR_OPERATION_CANCELLED,

    /**
     * @brief IO is in error.
     */
	ERR_IO_ERROR,

	/**
	 * @brief Storage device is configured as write protected.
	 */
	ERR_STORAGE_WRITE_PROTECTED,

	/**
	 * Volume is configured as write protected.
	 */
	ERR_VOLUME_WRITE_PROTECTED,

	/**
	 * @brief The file descriptor is in staled state.
	 */
	ERR_STALE_FILE_DESCRIPTOR,

	/**
	 * @brief Error invalid file descriptor permissions
	 */
	ERR_INVALID_WRITE_ACCESS,
	/**
	 * @brief  Permitted atomic range is exceeded.
	 */
	ERR_OUT_OF_ATOMIC_RANGE,

	/**
	 * @brief Out of reserved space.
	 */
	ERR_NO_AVAILABLE_SPACE,

	/**
	 * @brief The directory entry name is out of allowed range.
	 */
	ERR_TOO_LONG_DIRECTORY_ENTRY,

	/**
	 * @brief Given mount point (file system) is invalid.
	 */
	ERR_INVALID_MOUNTPOINT,

    /**
	 * @brief Indicates that internal data structure for the particular block device or virtual device is corrupted.
	 */
	ERR_BLOCK_DEV_CORRUPTED,

} eError;

/**
 * @typedef gzis_status_gt
 */
typedef enum eError gzis_status_gt;

/**
 * @brief Indicates system state.
 */
typedef enum eSystemState {
    /**
     * @brief The state which begins by module power-on and continues during
     * system initialization. MODULE_INIT state ends with the completion of the
     * system initalization threads. (for GzIS_653)
     */
    MODULE_INIT = 0,

    /**
     * @brief The GzIS_POSIX equivalent of the state #MODULE_INIT.
     */
    OS_INIT = 0,

    /**
     * @brief The state which contains execution in kernel space (e.g., system functions,
     * syscalls, timers, callbacks, drivers, exceptions). (for GzIS_653)
     */
    MODULE_FUNCTION = 1,

    /**
     * @brief The GzIS_POSIX equivalent of the state #MODULE_FUNCTION.
     */
    OS_EXECUTION = 1,

    /**
     * @brief It represents the execution state in user space (e.g., address space).
     */
    PARTITION_EXECUTION = 2,

    /**
     * @brief The GzIS_POSIX equivalent of the state #PARTITION_EXECUTION.
     */
    USER_CODE_EXECUTION = 2,

    /**
     * @brief The number of system states.
     */
    SYSSTATE_COUNT = 3
} eSystemState;

/**
 * @typedef eSystemState system_state_gt 
 */
typedef enum eSystemState system_state_gt;

/**
 * @brief Defines the health monitor error types.
 */
typedef enum eSysErrorTypes 
{
    /**
     * @brief The error condition which occurs as a result of exceeding the
     * deadline time of the ARINC 653 process.
     */
    SYSERROR_DEADLINE_MISSED = 0,

    /**
     * @brief The error condition which is triggered by user program.
     */
    SYSERROR_APPLICATION_ERROR = 1,

    /**
     * @brief The error condition which occurs as a result of an integer overflow, 
     * a floating point exception or an attempt to divide by zero.
     */
    SYSERROR_NUMERIC_ERROR = 2,
    /**
     * @brief The error condition which occurs as a result of an invalid system call, 
     * address alignment error or an attempt to an illegal operation.
     */
    SYSERROR_ILLEGAL_REQUEST = 3,

    /**
     * @brief The error condition which occurs as a result of stack overflow.
     */
    SYSERROR_STACK_OVERFLOW = 4,

    /**
     * @brief The error condition which occurs as a result of an erroneous
     * memory segmentation or supervisor privilege error.
     */
    SYSERROR_MEMORY_VIOLATION = 5,

    /**
     * @brief The error condition which occurs as result of a parity error, 
     * a result of an invalid I/O access or when there is no physical
     * address corresponding to the virtual address.
     */
    SYSERROR_HARDWARE_FAULT = 6,

    /**
     * @brief The error condition which occurs as result of a power interrupt or a power fail.
     */
    SYSERROR_POWER_FAIL = 7,

    /**
     * @brief The error condition which occurs during triggering health monitor
     * as a result of non-returnable errors in the kernel.
     */
    SYSERROR_OS_REPORT_ERROR = 8,

    /**
     * @brief The error condition which occurs as a result of an error
     * condition which not prevent the system to continue to execute.
     */
    SYSERROR_OS_LOG_MESSAGE = 9,

    /**
     * @brief The error condition which occurs as a result of an error which
     * requires restarting the partition not by health monitor.
     */
    NOT_HM_CALL = 100,
} eSysErrorTypes;

/**
 * @typedef eSysErrorTypes system_error_gt
 */
typedef enum eSysErrorTypes system_error_gt;

/*---------------------------------------------------------------------------*/
/*           CONSTRAINT DEFINITIONS                                          */
/*---------------------------------------------------------------------------*/

/** GZIS types' maximum and minimum defines */
#define GZIS_UINT1_MAX (0xFFU)
#define GZIS_UINT1_MIN (0)

#define GZIS_UINT2_MAX (0xFFFFU)
#define GZIS_UINT2_MIN (0)

#define GZIS_UINT4_MAX (0xFFFFFFFFU)
#define GZIS_UINT4_MIN (0)

#define GZIS_UINT8_MAX (0xFFFFFFFFFFFFFFFFULL)
#define GZIS_UINT8_MIN (0)

#define GZIS_INT1_MAX  (0x7F)
#define GZIS_INT1_MIN  (0x80)

#define GZIS_INT2_MAX  (0x7FFF)
#define GZIS_INT2_MIN  (0x8000)

#define GZIS_INT4_MAX  (0x7FFFFFFF)
#define GZIS_INT4_MIN  (0x80000000)

#define GZIS_INT8_MAX  (0x7FFFFFFFFFFFFFFFLL)
#define GZIS_INT8_MIN  (0x8000000000000000LL)

#if 8 == __SIZEOF_LONG__

#define GZIS_UVAL_MAX  GZIS_UINT8_MAX
#define GZIS_UVAL_MIN  GZIS_UINT8_MIN

#define GZIS_VAL_MAX   GZIS_INT8_MAX
#define GZIS_VAL_MIN   GZIS_INT8_MIN

#else

#define GZIS_UVAL_MAX  GZIS_UINT4_MAX
#define GZIS_UVAL_MIN  GZIS_UINT4_MIN

#define GZIS_VAL_MAX   GZIS_INT4_MAX
#define GZIS_VAL_MIN   GZIS_INT4_MIN

#endif



#ifdef __CHAR_UNSIGNED__
#define GZIS_BYTE_MAX  (byte_gt)(-1)
#define GZIS_BYTE_MIN  (0)
#else
#define GZIS_BYTE_MAX  (127)
#define GZIS_BYTE_MIN  (-GZIS_BYTE_MAX - 1)
#endif


/**
 * @brief A unsigned type that is used to define architecture-independent
 * parameters.
 * @remarks The size of the type long is equal to the word size of the CPU
 * architecture.
 * @deprecated Use uval_gt instead.
 */
typedef unsigned long VAL;

/**
 * @brief A signed type that is used to define architecture-independent
 * parameters.
 * @remarks The size of the type long is equal to the word size of the CPU
 * architecture.
 * @deprecated Use val_gt instead.
 */
typedef long SVAL;

/**
 * @brief A pointer type that is used to define architecture-independent
 * address pointer.
 * @deprecated Use addr_gt instead.
 */
typedef void * ADDR;

/**
 * @brief A pointer type that is used to define architecture-independent
 * pointer which points to physical address.
 * @deprecated 
 */
typedef void* PHYS_ADDR;

typedef VAL PHYS_SIZE;
typedef VAL PHYS_VAL;

#ifdef _GZIS_HIGH_MEM
/**
 * @brief Unsigned integer types that are 8 byte in size.
 * @remarks These types are defined for 4080 architecture.
 * @deprecated 
 */
typedef UINT8 PHYS_ADDR_64;
typedef UINT8 PHYS_SIZE_64;
typedef UINT8 PHYS_VAL_64;
#else
typedef PHYS_ADDR PHYS_ADDR_64;
typedef PHYS_SIZE PHYS_SIZE_64;
typedef PHYS_VAL  PHYS_VAL_64;
#endif

typedef PHYS_SIZE_64 MEM_PROFILE_SIZE;

/**
 * @brief Memory reservation types.
 * @deprecated 
 */
typedef enum RESERV_TYPE {
    /**
     * @brief Indicates memory reservation.
     */
    KERNEL_MEMORY_RESERVATION,

    /**
     * @brief Indicates driver reservation.
     */
    KERNEL_DRIVER_RESERVATION
} RESERV_TYPE;

/**
 * @brief A pointer type that is used to define architecture-independent
 * pointer which points to virtual address.
 * @deprecated 
 */
typedef void * VIRT_ADDR;

/**
 * @brief A double type that is 8 byte in size.
 * @deprecated 
 */
typedef double DOUBLE8;

/**
 * @brief GzIS functions' return data type.
 * @deprecated 
 */
typedef enum eError GZIS_ERR;

/**
 * @brief A pointer type that is used to define architecture-independent
 * pointer which points to physical address.
 */
typedef PHYS_ADDR phys_addr_gt;

/**
 * @brief A pointer type that is used to define architecture-independent
 * pointer which points to virtual address.
 */
typedef VIRT_ADDR virt_addr_gt;

/**
 * @brief A unsigned type that is used to define architecture-independent
 * parameters.
 * @remarks The size of the type long is equal to the word size of the CPU
 * architecture.
 */
typedef VAL size_gt;

/**
 * @brief A signed type that is used to define offset values.
 * @remarks The size of the type long is equal to the word size of the CPU
 * architecture.
 */
typedef SVAL off_gt;

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _TARGET_INC_GZIS_GZISTYPES_H_ */
