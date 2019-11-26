FiletreeRambler

Small C++ header-only utility for fast loading and processing of files from filesystem.

Usage:
1) set settings via ftr::settings_t struct (whether to check subdirs and maximal subdir depth, allowed file extensions, ignored file extensions, filename check (e.g. some pattern)), leave unused properties unset.
2) set number of processing threads via ftr::settings_t::max_threads struct field (make sure to guarantee threadsafe processing function in this case), default value is 1 (single thread)
3) create a processing func ftr::proc_func_t (don't use mutex if not neccessary)
4) call ftr::scan(...) with path to seek files, proccessing func and settings; call ftr:scan(...) without processing func to count files and dirs

Workflow:
1) ftr::scan(...) first passes through filesystem according to provided settings and lists files to process
2) listed files are processed (non-blocking in case of multiple process threads)
3) running time, number of processed (and skipped) files and dirs is returned to caller