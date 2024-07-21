#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

// void depth_estimation(int img1[][256],int img2[][256],int img3[][256]){
//     int i,j,a,b;
//     int d,d2,dmin,dt;

//     for (j=10; j < 256-10;j++){
//         for (i=10 ;i<256-10;i++){
//             d2=0;dmin=999999;

//             for (d = 0; d <=30; d++){
//                 dt=0;
//                 for (b=-1;b<=1;b++){
//                     for (a=-1;a<=1;a++){
//                         if (img1[j+b][i+a]!=img2[j+b][i+a+d]){
//                             dt ++;
//                         }                       
//                     }   
//                 } 
//                 if (dmin > dt ) {
//                     d2 = d;
//                     dmin = dt;
//                 }
//             }    
//         img3[j][i] = d2;
//         }
//     }
// }
void depth_estimation(int img1[][256], int img2[][256], int img3[][256]) {
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