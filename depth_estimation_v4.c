#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
// #include "depth_algorithm.h"

void img_out(const char *n,int img[][256]){
        
    FILE *fp;
    int i,j;

    fp = fopen(n,"w");
    fprintf(fp, "P2\n256 256\n255\n");
    for (j=0;j<256;j++){
        for (i=0;i<256;i++){
            fprintf(fp, "%d ", img[j][i]);
         }
         fprintf(fp, "\n");
    }
    fclose(fp);
}

void img_in(const char *n,int img[][256]){
        
    FILE *fp;
    int i,j;
    char tmps[256],str[4024],*tok;

    if ((fp = fopen(n,"r"))==NULL){
        printf("Cannot open %s file!!\n",n);
        exit(0);
    };
    fscanf(fp, "%s",tmps);
    fscanf(fp, "%d %d",&i,&j);
    fscanf(fp, "%d ",&i);
    j=0;
    while(fgets(str, 4024, fp) != NULL) {
        tok = strtok( str, " ");
        i=0;
        img[j][i++]=atoi(tok);
        while( tok != NULL ){
            img[j][i]=atoi(tok);
            tok = strtok( NULL, " \n" ); 
            i++;
        }
        j++;
	}
    fclose(fp);
}

/////////////////depth estimation //////////////////
///////  修正はここから
void depth_estimation(int img1[][256], int img2[][256], int img3[][256]) {

    int IMG_SIZE = 256;
    int WIN_SIZE = 5;
    int MAX_DISP= 30;
    int P1 =10;// Penalty for disparity difference of 1
    int P2 =150 ;// Penalty for larger disparity differences
    int DIRECTIONS =8;

    int directions[8][2] = {
        {0, 1}, 
        {1, 0}, 
        {1, 1}, 
        {1, -1},
        {0, -1}, 
        {-1, 0}, 
        {-1, -1}, 
        {-1, 1}
    };

    // int i, j, d, a, b;

    int half_win = WIN_SIZE / 2;
    int cost[256][256][30 + 1];
    int aggr_cost[256][256][30 + 1];
    int disp_range = MAX_DISP + 1;
    
    // Compute initial cost (SAD)
    #pragma omp parallel for collapse(2)
    for (int j = half_win; j < IMG_SIZE - half_win; j++) {
        for (int i = half_win; i < IMG_SIZE - half_win; i++) {
            for (int d = 0; d <= MAX_DISP; d++) {
                int sad = 0;
                for (int y = -half_win; y <= half_win; y++) {
                    for (int x = -half_win; x <= half_win; x++) {
                        int ref_pixel = img1[j + y][i + x];
                        int tgt_pixel = (i + x + d < IMG_SIZE) ? img2[j + y][i + x + d] : 0;
                        sad += abs(ref_pixel - tgt_pixel);
                    }
                }
                cost[j][i][d] = sad;
            }
        }
    }

    // Initialize aggregated cost with initial cost
    #pragma omp parallel for collapse(2)
    for (int d = 0; d < disp_range; d++) {
        for (int j = 0; j < IMG_SIZE; j++) {
            for (int i = 0; i < IMG_SIZE; i++) {
                aggr_cost[j][i][d] = cost[j][i][d];
            }
        }
    }
    // Aggregate costs along multiple directions
    for (int dir = 0; dir < DIRECTIONS; dir++) {
        int dx = directions[dir][0];
        int dy = directions[dir][1];

        int start_x = (dx > 0) ? 0 : IMG_SIZE - 1;
        int end_x = (dx > 0) ? IMG_SIZE : -1;
        int start_y = (dy > 0) ? 0 : IMG_SIZE - 1;
        int end_y = (dy > 0) ? IMG_SIZE : -1;

        for (int j = start_y; j != end_y; j += dy) {
            for (int i = start_x; i != end_x; i += dx) {
                for (int d = 0; d < disp_range; d++) {
                    int min_cost = aggr_cost[j][i][d];
                    if (i - dx >= 0 && i - dx < IMG_SIZE && j - dy >= 0 && j - dy < IMG_SIZE) {
                        min_cost = aggr_cost[j - dy][i - dx][d];

                        if (d > 0) {
                            min_cost = min_cost < (aggr_cost[j - dy][i - dx][d - 1] + P1) ? min_cost : (aggr_cost[j - dy][i - dx][d - 1] + P1);
                        }

                        if (d < disp_range - 1) {
                            min_cost = min_cost < (aggr_cost[j - dy][i - dx][d + 1] + P1) ? min_cost : (aggr_cost[j - dy][i - dx][d + 1] + P1);
                        }

                        min_cost = min_cost < (aggr_cost[j - dy][i - dx][d] + P2) ? min_cost : (aggr_cost[j - dy][i - dx][d] + P2);
                    }
                    aggr_cost[j][i][d] = cost[j][i][d] + min_cost;
                }
            }
        }
    }

    // Select disparity with minimum aggregated cost
    #pragma omp parallel for collapse(2)
    for (int j = 0; j < IMG_SIZE; j++) {
        for (int i = 0; i < IMG_SIZE; i++) {
            int best_disparity = 0;
            int min_cost = aggr_cost[j][i][0];

            for (int d = 1; d < disp_range; d++) {
                if (aggr_cost[j][i][d] < min_cost) {
                    min_cost = aggr_cost[j][i][d];
                    best_disparity = d;
                }
            }

            img3[j][i] = best_disparity;
        }
    }

}

// void depth_estimation(int img1[][256], int img2[][256], int img3[][256]){
//         /*
//     Sum of Absolute Differences (SAD) or Sum of Squared Differences (SSD), which can be more efficient than the pixel-by-pixel comparison used previously
//     Explanation:
//         Window Size: Use a window of size WIN_SIZE for computing SAD, centered around each pixel.
//         Sum of Absolute Differences (SAD): For each pixel in the image, compute the SAD for different disparities and choose the disparity with the smallest SAD.
//         Handling Boundaries: Ensure that the index does not go out of bounds when computing SAD.
//     Optimizations:
//         Window Size: Using a small window size reduces the computation compared to a 3x3 window around each pixel.
//         Sum of Absolute Differences (SAD): SAD is often faster to compute than a pixel-by-pixel comparison with a counter.
//         Memory Access Pattern: Improved spatial locality by accessing consecutive pixels in memory.
//     */
//     int IMG_SIZE = 256;
//     int WIN_SIZE = 5;
//     int MAX_DISP = 30;

//     int half_win = WIN_SIZE / 2;
//     int i, j, d, x, y;

//     #pragma omp parallel for private(i, j, d, x, y)
//     for (j = half_win; j < IMG_SIZE - half_win; j++) {
//         for (i = half_win; i < IMG_SIZE - half_win; i++) {
//             int best_disparity = 0;
//             int min_sad = 999999;

//             for (d = 0; d <= MAX_DISP; d++) {
//                 int sad = 0;

//                 for (y = -half_win; y <= half_win; y++) {
//                     for (x = -half_win; x <= half_win; x++) {
//                         int ref_pixel = img1[j + y][i + x];
//                         int tgt_pixel = (i + x + d < IMG_SIZE) ? img2[j + y][i + x + d] : 0;
//                         sad += abs(ref_pixel - tgt_pixel);
//                     }
//                 }

//                 if (sad < min_sad) {
//                     min_sad = sad;
//                     best_disparity = d;
//                 }
//             }

//             img3[j][i] = best_disparity;
//         }
//     }
// }
///////  修正はここまで
////////////////////////////////////////////////



int main() {
    
    int i,j,k,error=0;
    char str[256]={0};
    double ave_err=0,ave_time=0;

    int fnum=10;  /* 画像ペア数　入力ファイルの数を変更した場合ここも修正が必要　*/ 
    char imname[][256]={"","dot01","dot02","dot03","dot04","dot05","dot06","dot07","dot08","dot09","dot10"}; /* 入力ステレオ画像名*/


    int img1[256][256],img2[256][256]; /* 入力ステレオ画像*/
    int img_gt[256][256]; /* 正解データ画像*/
    int img3[256][256]={0}; /* 推定結果画像*/
    
    clock_t start_clock, end_clock;

    k=1;
    while (k <= fnum){
        strcpy(str,"dot2/");
        strcat(str,imname[k]);
        img_in(strcat(str,"-1.pgm"),img1);

        strcpy(str,"dot2/");
        strcat(str,imname[k]);
        img_in(strcat(str,"-2.pgm"),img2);

        strcpy(str,"dot2/");
        strcat(str,imname[k]);
        img_in(strcat(str,"-gt.pgm"),img_gt);

        start_clock = clock();
        
        depth_estimation(img1,img2,img3);
        
        end_clock = clock();

        /* 正答率判定　*/
        error = 0;
        for (j=0;j<256;j++){
            for (i=0;i<256;i++){
                if (img3[j][i] != img_gt[j][i] )error ++; 
            }
        }

        printf("%s -> Error Rate: %lf time: %f\n",imname[k],error/(j*i*1.0), (double)(end_clock - start_clock) / CLOCKS_PER_SEC);
        strcpy(str,"res/");
        strcat(str,imname[k]);
        img_out(strcat(str,"-res.pgm"),img3);
        ave_err +=error/(j*i*1.0);
        ave_time += (double)(end_clock - start_clock) / CLOCKS_PER_SEC;
        k++;
    }

    printf("Average Error Rate: %lf Average time: %lf \n",ave_err/(k-1),ave_time/(k-1)  );

} 