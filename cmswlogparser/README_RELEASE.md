# CMSW Log Parser

## Compatible Versions
MLCMSW v1.0.0

CLCMSW v1.1.1

PLCMSW v1.0.0

## Overview

In IPPP embedded systems, logs are generated on PowerPC (Big-Endian) architecture. However, most modern data analysis workstations (Windows/Intel/AMD) operate on Little-Endian architecture. Attempting to read these files directly results in corrupted or nonsensical data.

cmswlogparser automates the process of:

    Reading raw binary log files.
    Performing Endianness conversion (Big-Endian →→ Little-Endian).
    Parsing the binary structures into human-readable CSV format.

## Usage

### Parse a single file

To parse a specific log file, run:

```bash
cmswlogparser.exe -i <input_folder>/<input_file> --o <output_folder>
```

Example:

```bash
cmswlogparser.exe -i input/clcmsw_detections_31_10_x_file_0.log --o output
```

### Batch process all files

To parse all .log files within the input/ directory, use the --all flag:

```bash
rn.bat --all
```
warning: There shall be at least one .log file in the input folder. 

## Input and Output Files

The tool processes log files according to the following naming structure:

| INPUT | | OUTPUT |
| :--- | :--- | :--- |
| `adcu_<LRU_ID>_x_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_ADCU_CBIT_<LRU_ID>_<FILE_NO>.csv, LOG_ADCU_PBIT_<LRU_ID>_<FILE_NO>.csv, LOG_ADCU_DTNES_<LRU_ID>_<FILE_NO>.csv` |
| `chassis_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_CHASSIS_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `clcmsw_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_CL_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `clcmsw_detections_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `clcmsw_detections_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log.csv` |
| `dtnirsw_<LRU_ID>_x_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_DTN_IRSW_EMERGENCY_<LRU_ID>_<FILE_NO>.csv, LOG_DTN_IRSW_HEALTH_MON_<LRU_ID>_<FILE_NO>.csv, LOG_DTN_IRSW_PORT_MON_<LRU_ID>_<FILE_NO>.csv, LOG_DTN_IRSW_PARTIAL_PORT_MON_<LRU_ID>_<FILE_NO>.csv, LOG_DTN_IRSW_STATUS_MON_<LRU_ID>_<FILE_NO>.csv` |
| `dtnes_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_DSM_DTNES_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `dtnsw_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_DSM_DTNSW_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `exception_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_DSM_EXCEPTION_STATUS_<LRU_ID>_<SLOT_ID>.csv, LOG_DSM_EXCEPTION_MAJOR_SYNC_ERROR_<LRU_ID>_<SLOT_ID>.csv` |
| `ipmc_dpm_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_IPMC_DPM_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `ipmc_gpm_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_IPMC_GPM_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `ipmc_hsm_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_IPMC_HSM_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `ipmc_hum_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_IPMC_HUM_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `ipmc_iocm_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_IPMC_IOCM_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `ipmc_psm_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_IPMC_PSM_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `ipmc_smmm_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_IPMC_SMMM_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `mlcmsw_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_ML_STATUS_<LRU_ID>_<SLOT_ID>.csv, LOG_ML_DTN_ES_MONITORING_<LRU_ID>_<SLOT_ID>.csv, LOG_ML_RTOS_EXCEPTIONS_<LRU_ID>_<SLOT_ID>.csv` |
| `mlcmsw_detections_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `mlcmsw_detections_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log.csv` |
| `mmplcmsw_detections_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `mmplcmsw_detections_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log.csv` |
| `mmplcmsw_mmpstatus_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_MMP_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `mmplcmsw_state_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_PL_STATUS_<LRU_ID>_<SLOT_ID>.csv` |
| `smmm_<LRU_ID>_<SLOT_ID>_x_file_<FILE_NO>.log` | $\longrightarrow$ | `LOG_SMMM_STATUS_<LRU_ID>_<SLOT_ID>.csv` |




