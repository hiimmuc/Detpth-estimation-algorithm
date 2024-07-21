#include "depth_algorithm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void depth_estimation_base(int img1[][256],int img2[][256],int img3[][256]){
    int i,j,a,b;
    int d,d2,dmin,dt;

    for (j=10; j < 256-10;j++){
        for (i=10 ;i<256-10;i++){
            d2=0;dmin=999999;

            for (d = 0; d <=30; d++){
                dt=0;
                for (b=-1;b<=1;b++){
                    for (a=-1;a<=1;a++){
                        if (img1[j+b][i+a]!=img2[j+b][i+a+d]){
                            dt ++;
                        }                       
                    }   
                } 
                if (dmin > dt ) {
                    d2 = d;
                    dmin = dt;
                }
            }    
        img3[j][i] = d2;
        }
    }
}

void depth_estimation_v1(int img1[][256], int img2[][256], int img3[][256]) {
    /*
    optimize speed
    */
    int i, j, a, b;
    int d, d2, dmin, dt;
    
    for (j = 10; j < 246; j++) {
        for (i = 10; i < 246; i++) {
            d2 = 0;
            dmin = 999999;
            
            for (d = 0; d <= 30; d++) {
                dt = 0;
                for (b = -1; b <= 1; b++) {
                    for (a = -1; a <= 1; a++) {
                        int idx1 = img1[j + b][i + a];
                        int idx2 = img2[j + b][i + a + d];
                        dt += (idx1 != idx2);
                    }
                }
                if (dmin > dt) {
                    d2 = d;
                    dmin = dt;
                }
            }
            img3[j][i] = d2;
        }
    }
}


void depth_estimation_v2(int img1[][256], int img2[][256], int img3[][256]) {
    /*
    Sum of Absolute Differences (SAD) or Sum of Squared Differences (SSD), which can be more efficient than the pixel-by-pixel comparison used previously
    Explanation:
        Window Size: Use a window of size WIN_SIZE for computing SAD, centered around each pixel.
        Sum of Absolute Differences (SAD): For each pixel in the image, compute the SAD for different disparities and choose the disparity with the smallest SAD.
        Handling Boundaries: Ensure that the index does not go out of bounds when computing SAD.
    Optimizations:
        Window Size: Using a small window size reduces the computation compared to a 3x3 window around each pixel.
        Sum of Absolute Differences (SAD): SAD is often faster to compute than a pixel-by-pixel comparison with a counter.
        Memory Access Pattern: Improved spatial locality by accessing consecutive pixels in memory.
    */
    int IMG_SIZE = 256;
    int WIN_SIZE = 3;
    int MAX_DISP = 30;
    int half_win = WIN_SIZE / 2;
    int i, j, d, x, y;

    #pragma omp parallel for private(i, j, d, x, y)
    for (j = half_win; j < IMG_SIZE - half_win; j++) {
        for (i = half_win; i < IMG_SIZE - half_win; i++) {
            int best_disparity = 0;
            int min_sad = 999999;

            for (d = 0; d <= MAX_DISP; d++) {
                int sad = 0;

                for (y = -half_win; y <= half_win; y++) {
                    for (x = -half_win; x <= half_win; x++) {
                        int ref_pixel = img1[j + y][i + x];
                        int tgt_pixel = (i + x + d < IMG_SIZE) ? img2[j + y][i + x + d] : 0;
                        sad += abs(ref_pixel - tgt_pixel);
                    }
                }

                if (sad < min_sad) {
                    min_sad = sad;
                    best_disparity = d;
                }
            }

            img3[j][i] = best_disparity;
        }
    }
}


// Helper function to calculate absolute difference
int abs_diff(int a, int b) {
    return abs(a - b);
}

void depth_estimation_sgm(int img1[][256], int img2[][256], int img3[][256]) {
    int IMG_SIZE =  256;
    int MAX_DISP = 30;
    int P1 = 10; // Penalty for disparity change by 1
    int P2 = 150 ;// Penalty for larger disparity changes
    
    int i, j, d;
    
    int Lr[4][256][256][30 + 1] = {0};
    int C[256][256][30 + 1] = {0};
    int S[256][256][30 + 1] = {0};

    // Step 1: Compute initial matching cost
    for (j = 0; j < IMG_SIZE; j++) {
        for (i = 0; i < IMG_SIZE; i++) {
            for (d = 0; d <= MAX_DISP; d++) {
                if (i + d < IMG_SIZE) {
                    C[j][i][d] = abs(img1[j][i] - img2[j][i + d]);
                } else {
                    C[j][i][d] = INT_MAX;
                }
            }
        }
    }

    // Step 2: Cost aggregation along different paths
    for (j = 0; j < IMG_SIZE; j++) {
        for (i = 0; i < IMG_SIZE; i++) {
            for (d = 0; d <= MAX_DISP; d++) {
                // Aggregate costs from different directions (left-right, right-left, top-bottom, bottom-top)
                for (int dir = 0; dir < 4; dir++) {
                    int i_prev = i, j_prev = j;
                    if (dir == 0 && i > 0) i_prev--;       // left-right
                    if (dir == 1 && i < IMG_SIZE - 1) i_prev++; // right-left
                    if (dir == 2 && j > 0) j_prev--;       // top-bottom
                    if (dir == 3 && j < IMG_SIZE - 1) j_prev++; // bottom-top

                    int min_prev_L = INT_MAX;
                    if (i_prev >= 0 && i_prev < IMG_SIZE && j_prev >= 0 && j_prev < IMG_SIZE) {
                        for (int dp = 0; dp <= MAX_DISP; dp++) {
                            min_prev_L = (Lr[dir][j_prev][i_prev][dp] < min_prev_L) ? Lr[dir][j_prev][i_prev][dp] : min_prev_L;
                        }
                    }

                    for (int dp = 0; dp <= MAX_DISP; dp++) {
                        int cost = C[j][i][dp];
                        int penalty = (dp == d) ? 0 : ((abs(dp - d) == 1) ? P1 : P2);
                        Lr[dir][j][i][dp] = cost + ((i_prev >= 0 && i_prev < IMG_SIZE && j_prev >= 0 && j_prev < IMG_SIZE) ? Lr[dir][j_prev][i_prev][dp] : 0) - min_prev_L + penalty;
                    }
                }
            }
        }
    }

    // Step 3: Sum the aggregated costs
    for (j = 0; j < IMG_SIZE; j++) {
        for (i = 0; i < IMG_SIZE; i++) {
            for (d = 0; d <= MAX_DISP; d++) {
                S[j][i][d] = 0;
                for (int dir = 0; dir < 4; dir++) {
                    S[j][i][d] += Lr[dir][j][i][d];
                }
            }
        }
    }

    // Step 4: Select the disparity with the minimum cost
    for (j = 0; j < IMG_SIZE; j++) {
        for (i = 0; i < IMG_SIZE; i++) {
            int best_d = 0;
            int min_cost = INT_MAX;
            for (d = 0; d <= MAX_DISP; d++) {
                if (S[j][i][d] < min_cost) {
                    min_cost = S[j][i][d];
                    best_d = d;
                }
            }
            img3[j][i] = best_d;
        }
    }
}