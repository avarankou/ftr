#pragma once

#include "logger.h"
#include "file_utils.h"

#include <atomic>
#include <filesystem>
#include <list>
#include <mutex>
#include <sstream>
#include <thread>

namespace ftr
{
    using LL = logger::LOG_LEVEL_t;
    namespace fs = std::experimental::filesystem::v1;
    using proc_func_t = void(*)(const fs::path & entry, std::mutex &);
    using filename_check_func_t = bool(*)(const fs::path & fullpath);

    struct settings_t
    {
        settings_t()
            : check_subdirs(false)
            , max_subdir_depth(0u)
            , max_threads(8u)
            , filename_check(nullptr)
        {
            if (max_threads == 0u)
                max_threads = 1u;
        }

        bool check_subdirs; /* whether to check subdirs */
        size_t max_subdir_depth; /* maximal depth of subdirs to check, set 0 for any depth */

        std::list<std::string> ext_list; /* file extensions (lowercase) to process, leave empty to process all */
        std::list<std::string> skip_ext_list; /* file extensions (lowercase) to skip, leave empty to process all */

        size_t max_threads; /* max number of processing threads */

        filename_check_func_t filename_check; /* additional check of filename */

        /* check if file's extension is listed in ext_list or skip_ext_list
        ** than check filename with filename_check if provided */
        bool check_file(const fs::path & file) const
        {
            if (!file.has_extension())
                return false;

            bool check = true;
            std::string ext(file.extension().string());
            to_lower(ext);

            if (!ext_list.empty())
                check = (std::find(ext_list.begin(), ext_list.end(), ext) != ext_list.end());

            if (check &&
                !skip_ext_list.empty())
                check = !(std::find(skip_ext_list.begin(), skip_ext_list.end(), ext) != skip_ext_list.end());

            if (check &&
                filename_check)
                check = filename_check(file);

            return check;
        }
    };

    struct count_t
    {
        count_t()
            : files_processed(0u)
            , files_skipped(0u)
            , dirs_num(0u) {}

        size_t files_processed;
        size_t files_skipped;
        size_t dirs_num;
        std::chrono::duration<double> duration;
    };

    /* avoid explicit call of utilities form ftr_inner namespace */
    namespace ftr_inner
    {
        struct file_processor_t
        {
            file_processor_t(const settings_t & settings)
                : settings(settings)
                , file_lists(nullptr)
                , file_list_idx(0u)
            {
                if (settings.max_threads == 1u)
                {
                    file_lists = new list_t();
                }
                else
                {
                    file_lists = new list_t[settings.max_threads];
                }
            }

            ~file_processor_t()
            {
                if (settings.max_threads == 1u)
                {
                    delete file_lists;
                }
                else
                {
                    delete[] file_lists;
                }
            }

            const settings_t settings;
            count_t count;

            void procees_dir(const fs::path & dir, proc_func_t func)
            {
                list_files(dir);

                if (settings.max_threads == 1u)
                {
                    for (const auto & file : *file_lists)
                        func(file, std::mutex());

                    return;
                }

                std::thread * threads = new std::thread[settings.max_threads];
                std::mutex resource_mutex;

                for (size_t i = 0u; i < settings.max_threads; ++i)
                    threads[i] = std::thread(thread_func, func, std::ref(resource_mutex), std::ref(file_lists[i]));

                resource_mutex.lock();
                logger::LOG_MSG(LL::Debug, "Waiting for processings threads.");
                resource_mutex.unlock();

                for (size_t i = 0u; i < settings.max_threads; ++i)
                    threads[i].join();

                delete[] threads;
            }

        private:
            using list_t = std::list<fs::path>;
            list_t * file_lists;
            size_t file_list_idx;

            void list_files(const fs::path & dir)
            {
                std::stringstream set;
                set << "\nRoot dir: " << dir
                    << "\nMax subdir level: " << (settings.check_subdirs ?
                    (settings.max_subdir_depth == 0 ? "any" : std::to_string(settings.max_subdir_depth)) : " skip subdirs")
                    << "\nAllowed file extensions: ";
                if (settings.ext_list.empty())
                    set << " any";
                else
                    for (const auto & ext : settings.ext_list)
                        set << ext << ' ';
                if (!settings.skip_ext_list.empty())
                {
                    set << "\nForbidden file extensions: ";
                    for (const auto & ext : settings.skip_ext_list)
                        set << ext << ' ';
                }
                set << "\nProcessing threads: " << settings.max_threads;
                logger::LOG_MSG(LL::Info, set.str());

                for (const auto & file : fs::directory_iterator(dir))
                {
                    bool has_ext = file.path().has_extension();

                    if (!has_ext &&
                        settings.check_subdirs)
                    {
                        scan_subdir(file, 1u);
                    }
                    else if (has_ext)
                    {
                        add_file(file);
                    }
                }

                std::stringstream res;
                res << count.files_processed << " files are detected in "
                    << count.dirs_num << " dirs while "
                    << count.files_skipped << " files are skipped.";
                logger::LOG_MSG(LL::Info, res.str());
            }

            void add_file(const fs::path & file)
            {
                if (settings.check_file(file))
                {
                    ++count.files_processed;
                    file_lists[file_list_idx++].push_back(file);
                    file_list_idx %= settings.max_threads;
                }
                else
                {
                    ++count.files_skipped;
                }
            }

            void scan_subdir(const fs::path & dir, size_t depth)
            {
                ++count.dirs_num;

                for (const auto & file : fs::directory_iterator(dir))
                {
                    bool has_ext = file.path().has_extension();

                    if (!has_ext &&
                        settings.max_subdir_depth == 0u ||
                        depth < settings.max_subdir_depth)
                    {
                        scan_subdir(file, ++depth);
                    }
                    else if (has_ext)
                    {
                        add_file(file);
                    }
                }
            }

            static void thread_func(proc_func_t func, std::mutex & resource_mutex, const list_t & file_list)
            {
                std::stringstream msg;
                msg << "Started thread with " << file_list.size() << " files.";
                resource_mutex.lock();
                logger::LOG_MSG(LL::Debug, msg.str());
                resource_mutex.unlock();

                for (const auto & file : file_list)
                    func(file, resource_mutex);
            }
        };
    }

    count_t scan(const fs::path & dir, proc_func_t func, const settings_t & settings)
    {
        auto start_time = std::chrono::steady_clock::now();

        ftr_inner::file_processor_t proc(settings);
        proc.procees_dir(dir, func);

        auto end_time = std::chrono::steady_clock::now();
        proc.count.duration = end_time - start_time;

        std::stringstream msg;
        msg << "Running time: " << proc.count.duration.count() << " secs.";
        logger::LOG_MSG(LL::Info, msg.str());

        return proc.count;
    }

    count_t scan(const std::string & dir, proc_func_t func, const settings_t & settings)
    {
        return scan(fs::path(dir), func, settings);
    }
}