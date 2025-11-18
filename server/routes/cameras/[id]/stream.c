#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> // برای تعریف pid_t
#include <unistd.h>    // برای توابع fork() و exec()
#include <sys/wait.h>

#define RTSP_URL "rtsp://admin:admin@192.168.1.239:554/stream"
// مسیر قابل دسترسی عمومی برای فایل خروجی
#define OUTPUT_FILE "web_snapshot.jpg" 
// آدرس URL برای فایل خروجی (همان آدرسی که مرورگر استفاده می کند)
#define IMAGE_URL "/web_snapshot.jpg" 
#define FFMPEG_PATH "ffmpeg" // اگر FFmpeg در path سیستم نباشد، مسیر کامل را اینجا قرار دهید

int main() {
    // 1. هدرهای HTTP برای بازگرداندن متن ساده (آدرس فایل)
    printf("Content-Type: text/plain\r\n");
    printf("Cache-Control: no-cache, no-store, must-revalidate\r\n");
    printf("\r\n");

    // 2. آماده سازی آرگومان های FFmpeg
    char *ffmpeg_args[] = {
        FFMPEG_PATH,
        "-y", // اجبار به بازنویسی فایل خروجی
        "-i",
        RTSP_URL,
        "-f", "image2",
        "-vframes", "1",
        "-q:v", "30",
        "-s", "320x240",
        "-nostats",
        "-loglevel", "quiet",
        "-c:v", "mjpeg",
        OUTPUT_FILE,
        NULL
    };
    
    
    pid_t pid = fork();

    if (pid == 0) {
        // Child Process: اجرای FFmpeg
        execvp(FFMPEG_PATH, ffmpeg_args);
        // در صورت شکست execvp
        exit(1);
    } else if (pid > 0) {
        // Parent Process: منتظر می‌ماند تا FFmpeg کارش تمام شود
        int status;
        waitpid(pid, &status, 0);

        // 4. بازگرداندن آدرس URL تصویر
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf(IMAGE_URL);
            return 0;
        } else {
            // در صورت شکست FFmpeg، یک پیام خطا ارسال شود
            fprintf(stderr, "FFmpeg failed to execute or returned an error.\n");
            printf("/error.jpg"); // یا یک آدرس فایل خطای ثابت
            return 1;
        }
    } else {
        // خطا در fork
        fprintf(stderr, "Fork failed.\n");
        return 1;
    }
}