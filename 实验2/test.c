#include <cstdio>
#include <cstring>
#include <algorithm>

#define WIN_SIZE 3
#define WIDTH 6
#define HEIGHT 6
#define HALF_SIZE (((WIN_SIZE) - 1) / 2)

#define POOL_SIZE 2
#define POOL_STRIDE 2
#define POOL_OUT_HEIGHT ((HEIGHT - POOL_SIZE) / POOL_STRIDE + 1)
#define POOL_OUT_WIDTH ((WIDTH - POOL_SIZE) / POOL_STRIDE + 1)


// 是否在图像范围内
int bounds_ok(const int y, int x)
{
    return (0 <= y && y < HEIGHT && 0 <= x && x < WIDTH);
}


/** 卷积操作
 *  这里如果是3*3，则使用的卷积核如下
 * -2  -1  0
 * -1  0   1
 *  0  1   2
 *
 * 应该是用来检测45度角的
 **/
int conv(int window[WIN_SIZE][WIN_SIZE], int y, int x)
{
    int result = 0;

    for (int i = -HALF_SIZE; i <= HALF_SIZE; i++)
        for (int j = -HALF_SIZE; j <= HALF_SIZE; j++)
            if (bounds_ok(y + i, x + j) == 1) {
                result += window[i + HALF_SIZE][j + HALF_SIZE] * (i + j);   // (i+j)巧妙地构造了上面的卷积核
            }

    return result;
}


// 卷积运算
void my_conv(const int in[], int out[]) {

    int line_buf[WIN_SIZE - 1][WIDTH];  // 2行WIDTH宽
    int window[WIN_SIZE][WIN_SIZE];     // 卷积核
    int up[WIN_SIZE];                   // 上移缓存

    memset(window, 0, sizeof(window));
    memset(line_buf, 0, sizeof(line_buf));
    memset(up, 0, sizeof(up));

    // 已经读入的数量
    int read_count = 0;

    // 输入前HALF_SIZE行
    for (int y = 0; y < HALF_SIZE; y++)
        for (int x = 0; x < WIDTH; x++)
            line_buf[y][x] = in[y*WIDTH+x];

    // 左下角HALF_SIZE+1个
    for (int x = 0; x < HALF_SIZE + 1; x++)
        line_buf[HALF_SIZE][x] = in[HALF_SIZE*WIDTH + x];

    // 初始化完成
    read_count = WIDTH * HALF_SIZE + HALF_SIZE + 1;

    // 把初始像素复制进入窗口
    for (int y = 0; y <= HALF_SIZE; y++)
        for (int x = 0; x <= HALF_SIZE; x++)
            window[y+1][x+1] = line_buf[y][x];

    // 记录输出位置
    int write_count = 0;

    // 开始卷积
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {

            // 执行卷积
            int val_out = conv(window, y, x);

            // 输出卷积结果
            out[write_count] = val_out;
            write_count++;

            // 窗口最右一列上移一位，up准备最右侧的缓存
            up[0] = line_buf[0][x+HALF_SIZE];
            for (int k = 1; k < WIN_SIZE - 1; k++)
                up[k] = line_buf[k - 1][x] = line_buf[k][x];

            // 读进输入值
            int val_in = 0;
            if (read_count < HEIGHT * WIDTH)
            {
                val_in = in[read_count];
                read_count++;
            }

            // 在输入的同时更新缓存，同时完成最右侧一列的最后一个元素
            up[WIN_SIZE - 1] = line_buf[WIN_SIZE - 2][x] = val_in;

            // 把窗口除了最右一列往左移1位
            for (int m = 0; m < WIN_SIZE; m++)
                for (int n = 0; n < WIN_SIZE - 1; n++)
                    window[m][n] = window[m][n + 1];

            // 更新最右一列窗口值
            for (int i = 0; i < WIN_SIZE; i++)
                window[i][WIN_SIZE - 1] = up[i];
        }
    }
}

// 池化操作
void my_pool(const int in[], int out[]){

    int img[HEIGHT][WIDTH]; // 将输入流转换为二位矩阵
    for(int i=0; i<HEIGHT; i++){
        for(int j=0; j<WIDTH; j++){
            img[i][j] = in[i*WIDTH + j];
        }
    }

    int temp;
    int write_count = 0;

    for(int j=0, tj=0; j<POOL_OUT_HEIGHT; j++, tj+=POOL_STRIDE){
        for(int i=0, ti=0; i<POOL_OUT_WIDTH; i++, ti+=POOL_STRIDE){

            temp = img[tj][ti];

            // 最大池化
            for(int oj=0; oj < POOL_SIZE; oj++){
                for(int oi=0; oi < POOL_SIZE; oi++){
                    if (bounds_ok(tj+oj, ti+oi)) {
                        temp = std::max(temp, img[tj+oj][ti+oi]);
                    }
                }
            }

            out[write_count] = temp;
            write_count++;
        }
    }
}


// 将流式输出转换为二位矩阵输出
void printMat(const int in[], int h, int w){
    for(int i=0; i<h; i++){
        for(int j=0; j<w; j++){
            printf("%d\t", in[i*w+j]);
        }
        printf("\n");
    }
}

// 融合卷积和池化操作
// 输入是图像大小
// 输出是池化后大小
void combine(const int in[], int out[]){
    int temp[HEIGHT * WIDTH];
    memset(temp, 0, sizeof(temp));

    printf("原始矩阵：\n");
    printMat(in, HEIGHT, WIDTH);

    my_conv(in, temp);

    printf("卷积输出：\n");
    printMat(temp, HEIGHT, WIDTH);

    my_pool(temp, out);

    printf("池化结果：\n");
    printMat(out, POOL_OUT_HEIGHT, POOL_OUT_WIDTH);
}



int main(){

    // 准备数据，创建一个 6*6的测试矩阵
    int in[HEIGHT][WIDTH] = {
            {-5, -4, -3, -2, -1, 0,},
            {-4, -3, -2, -1, 0, 1,},
            {-3, -2, -1, 0, 1, 2,},
            {-2, -1, 0, 1, 2, 3,},
            {-1, 0, 1, 2, 3, 4,},
            {0, 1, 2, 3, 4, 5,},
    };

    // 创建一个输出矩阵
    int out[HEIGHT * WIDTH];
    memset(out, 0, sizeof(out));

    // 把in转化为单行矩阵(流)
    int a[HEIGHT * WIDTH];
    memset(a, 0, sizeof(a));
    for(int i=0; i<HEIGHT; i++){
        for(int j=0; j<WIDTH; j++){
            a[i*WIDTH + j] = in[i][j];
        }
    }

    // 合并操作
    combine(a, out);

    return 0;
}
