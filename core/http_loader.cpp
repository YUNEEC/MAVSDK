#include "http_loader.h"
#include "curl_wrapper.h"

namespace dronelink {

HttpLoader::HttpLoader()
{
    start();
}

HttpLoader::~HttpLoader()
{
    stop();
}

void HttpLoader::start()
{
    _should_exit = false;
    _work_thread = new std::thread(work_thread, this);
}

void HttpLoader::stop()
{
    _should_exit = true;
    _work_queue.stop();
    if (_work_thread != nullptr) {
        _work_thread->join();
        delete _work_thread;
        _work_thread = nullptr;
    }
}

bool HttpLoader::download_sync(const std::string &url, const std::string &local_path)
{
    auto work_item = std::make_shared<DownloadItem>(url, local_path, nullptr);
    bool success = do_download(work_item);
    return success;
}

void HttpLoader::download_async(const std::string &url, const std::string &local_path,
                                const progress_callback_t &progress_callback)
{
    auto work_item = std::make_shared<DownloadItem>(url, local_path, progress_callback);
    _work_queue.enqueue(work_item);
}

bool HttpLoader::upload_sync(const std::string &target_url, const std::string &local_path)
{
    auto work_item = std::make_shared<UploadItem>(target_url, local_path, nullptr);
    bool success = do_upload(work_item);
    return success;
}

void HttpLoader::upload_async(const std::string &target_url, const std::string &local_path,
                              const progress_callback_t &progress_callback)
{
    auto work_item = std::make_shared<UploadItem>(target_url, local_path, progress_callback);
    _work_queue.enqueue(work_item);
}

void HttpLoader::work_thread(HttpLoader *self)
{
    while (!self->_should_exit) {
        auto item = self->_work_queue.dequeue();
        if (item == nullptr) {
            continue;
        }
        do_item(item);
    }
}

void HttpLoader::do_item(const std::shared_ptr<WorkItem> &item)
{
    auto download_item = std::dynamic_pointer_cast<DownloadItem>(item);
    if (nullptr != download_item) {
        do_download(download_item);
        return;
    }

    auto upload_item = std::dynamic_pointer_cast<UploadItem>(item);
    if (nullptr != upload_item) {
        do_upload(upload_item);
        return;
    }
}

bool HttpLoader::do_download(const std::shared_ptr<DownloadItem> &item)
{
    CurlWrapper curl_wrapper;
    bool success = curl_wrapper.download_file_to_path(item->get_url(), item->get_local_path(),
                                                      item->get_progress_callback());
    return success;
}

bool HttpLoader::do_upload(const std::shared_ptr<UploadItem> &item)
{
    CurlWrapper curl_wrapper;
    bool success = curl_wrapper.upload_file(item->get_target_url(), item->get_local_path(),
                                            item->get_progress_callback());
    if (success == false) {
        auto callback = item->get_progress_callback();
        if (nullptr != callback) {
            callback(100);
        }
    }

    return success;
}

bool HttpLoader::download_text_sync(const std::string &url, std::string &content)
{
    CurlWrapper curl_wrapper;
    bool success = curl_wrapper.download_text(url, content);
    return success;
}


} // namespace dronelink

