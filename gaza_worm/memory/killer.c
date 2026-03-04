#include <windows.h>
#include <stdio.h>

int main() {
    // استبدل 1234 برقم العملية (PID) الذي تجده في مدير المهام (Task Manager)
    DWORD processID = 1234; 

    // 1. فتح العملية بصلاحية الإنهاء (PROCESS_TERMINATE)
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processID);

    if (hProcess == NULL) {
        printf("خطأ: لا يمكن الوصول للعملية. تأكد من رقم الـ PID وصلاحيات المسؤول.\n");
        return 1;
    }

    // 2. إنهاء العملية
    if (TerminateProcess(hProcess, 0)) {
        printf("تم إغلاق العملية رقم %lu بنجاح.\n", processID);
    } else {
        printf("فشل إغلاق العملية. رمز الخطأ: %lu\n", GetLastError());
    }

    // 3. إغلاق المقبض لتحرير موارد النظام
    CloseHandle(hProcess);

    return 0;
}
/*#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h> // للتعامل مع رؤوس IP
#include <netinet/tcp.h> // للتعامل مع رؤوس TCP

int main() {
    int sock_raw;
    struct sockaddr_in saddr;
    unsigned char *buffer = (unsigned char *)malloc(65536);

    // 1. إنشاء Raw Socket لالتقاط كافة حزم IP
    sock_raw = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if(sock_raw < 0) {
        printf("خطأ في إنشاء السوكيت! (تحتاج صلاحيات Root)\n");
        return 1;
    }

    printf("بدء مراقبة الشبكة لاعتراض الدودة...\n");

    while(1) {
        int data_size = recvfrom(sock_raw, buffer, 65536, 0, NULL, NULL);
        if(data_size < 0) break;

        // 2. الوصول إلى محتوى البيانات (Payload)
        // نتخطى رأس الـ IP ورأس الـ TCP للوصول للبيانات الفعلية
        struct iphdr *iph = (struct iphdr *)buffer;
        unsigned short iphdrlen = iph->ihl*4;
        
        // البحث عن بصمة الدودة (مثلاً كلمة "WORM_PAYLOAD")
        char *payload = (char *)(buffer + iphdrlen + sizeof(struct tcphdr));
        
        if (strstr(payload, "WORM_CODE_XYZ") != NULL) {
            printf("\n⚠️ تم اكتشاف دودة شبكية من العنوان: %s\n", inet_ntoa(*(struct in_addr *)&iph->saddr));
            printf("إجراء: تم اعتراض الحزمة وتحليلها.\n");
            // هنا يمكن إضافة كود لقطع الاتصال أو إرسال تنبيه للـ Firewall
        }
    }

    return 0;
}
*/