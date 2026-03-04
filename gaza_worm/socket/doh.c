#include <stdio.h>
#include <curl/curl.h>

// نموذج بسيط لاستعلام DoH باستخدام مكتبة curl
int main(void) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {
        // تحديد رابط مزود الخدمة (مثلاً Cloudflare) مع اسم النطاق المراد البحث عنه
        // ملاحظة: هذا مجرد مثال تعليمي للرابط
        curl_easy_setopt(curl, CURLOPT_URL, "https://dns.cloudflare.com");
        
        // ضبط الرأس (Header) لقبول استجابة بتنسيق DNS json أو binary
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/dns-json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // تنفيذ الطلب
        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        // التنظيف
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    return 0;
}
