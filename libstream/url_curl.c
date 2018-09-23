#include "config.h"
#ifdef HAVE_CURL_CURL_H

#include <unistd.h>
#include <curl/curl.h>
#include "export.h"
#include "stream.h"
#include "compat.h"
#include "gettext.h"
VISIBILITY_ENABLE
#include "ttyrec.h"
VISIBILITY_DISABLE

#define USER_AGENT "libtermrec/"PACKAGE_VERSION

static void curl_r(int in, int out, const char *arg)
{
    CURL *curl_handle;
    CURLcode res;
    FILE *f = fdopen(out, "w");
    if (!f)
        return close(out), (void)0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, arg);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)f);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, USER_AGENT);
    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
        fprintf(f, "CURL failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl_handle);
    fclose(f);
}

static void curl_w(int in, int out, const char *arg)
{
    CURL *curl_handle;
    CURLcode res;
    FILE *f = fdopen(out, "r");
    if (!f)
        return close(out), (void)0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, arg);
    curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1);
    curl_easy_setopt(curl_handle, CURLOPT_READDATA, (void *)f);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, USER_AGENT);
    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
        fprintf(f, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl_handle);
    fclose(f);
}

int open_curl(const char* url, int mode, const char **error)
{
    if (mode == SM_REPREAD)
    {
        *error = "Watching CURL URLs is not (yet?) supported.";
        return -1;
    }
    else if (mode == SM_APPEND)
    {
        // There's CURLOPT_APPEND but it says it's for FTP only, FTP is dead.
        *error = "Appending via CURL is not (yet?) supported.";
        return -1;
    }

    if (mode&SM_WRITE)
        return filter(curl_w, -1, 1, url, error);
    else
        return filter(curl_r, -1, 0, url, error);
}
#endif
