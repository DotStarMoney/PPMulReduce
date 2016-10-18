#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define MAX_TEXT_LINE 256
#define MAX_COLUMN_HEIGHT 16
#define REDUCTION_SERIES_N 8
#define MAX_ADDER_BLOCK 256


char* strtok_noskip(char* str, char const* delims)
{
    static char*  src = NULL;
    char*         p;
    char*         ploc;
    char*         ret;

    ret = 0;

    if (str != NULL) src = str;

    if (src == NULL) return NULL;

    ploc = strpbrk(src, delims);
    if(ploc != NULL)
    {
        p = ploc;
        *p  = 0;
        ret = src;
        src = ++p;
    }
    else if(src)
    {
        if(*src != '\0'){
            ret = src;
            src = NULL;
        }
    }

    return ret;
}

char* trimWhitespace(char *str)
{
    char* end;

    while(isspace(*str)) str++;
    if(*str == 0) return str;

    end = str + strlen(str) - 1;
    while(end > str && isspace(*end)) end--;
    *(end+1) = 0;

    return str;
}

typedef struct bitColumn bitColumn;
struct bitColumn
{
    struct bitColumn*   prev_;
    struct bitColumn*   next_;
    int                 bitSource[MAX_COLUMN_HEIGHT];
    int                 addedSource[MAX_COLUMN_HEIGHT];
    int                 bitSource_n;
    int                 addedSource_n;
};

typedef struct adderBlock adderBlock;
struct adderBlock
{
    bool    isFull;
    int     carryOut;
    int     out;
    int     a;
    int     b;
    int     carryIn;
};

int main(int argc, char* argv[])
{
    FILE*       treefile;
    char        textLine[MAX_TEXT_LINE];
    adderBlock  blocks[MAX_ADDER_BLOCK];
    int         blocks_n;
    bitColumn*  head_;
    bitColumn*  tail_;
    bitColumn*  curColumn_;
    bitColumn*  temp_;
    char*       token;
    char        tokenCopy[MAX_TEXT_LINE];
    char*       bareToken;
    int         maxColumn;
    int         reductionSeries[16];
    int         i;
    int         dj;
    int         dj_i;
    int         totalHeight;
    int         generateBit;
    int         minGenerate;
    int         row;

    if(argc < 2)
    {
        printf("Dadda Reduce. Generates Dadda reduction of input .csv file.\n\n");
        printf("Usage: <filename>");
        exit(1);
    }

    if(!(treefile = fopen(argv[1], "r")))
    {
        printf("ERROR: Unable to open file.");
        exit(1);
    }

    reductionSeries[0] = 2;
    for(i = 1; i < REDUCTION_SERIES_N; i++)
    {
        reductionSeries[i] = (int) floor(reductionSeries[i - 1]*1.5);
    }

    head_ = 0;
    tail_ = 0;
    maxColumn = 0;
    generateBit = 0;
    while(fgets(textLine, MAX_TEXT_LINE, treefile))
    {
        token = strtok_noskip(textLine, ",");

        curColumn_ = tail_;
        while(token)
        {
            strcpy(tokenCopy, token);
            bareToken = trimWhitespace(tokenCopy);
            if(!curColumn_)
            {
                tail_ = (bitColumn*) malloc(sizeof(bitColumn));
                head_ = tail_;
                head_->prev_ = 0;
                head_->next_ = 0;
                head_->bitSource_n = 0;
                head_->addedSource_n = 0;
                curColumn_ = head_;
            }

            if(*bareToken)
            {
                curColumn_->bitSource[curColumn_->bitSource_n] = atoi(bareToken);
                if(curColumn_->bitSource[curColumn_->bitSource_n] > generateBit)
                {
                    generateBit = curColumn_->bitSource[curColumn_->bitSource_n];
                }
                curColumn_->bitSource_n++;

                if(curColumn_->bitSource_n > maxColumn) maxColumn = curColumn_->bitSource_n;
            }

            if(!curColumn_->prev_)
            {
                temp_ = head_;
                head_ = (bitColumn*) malloc(sizeof(bitColumn));
                head_->prev_ = 0;
                head_->next_ = temp_;
                temp_->prev_ = head_;
                head_->bitSource_n = 0;
                head_->addedSource_n = 0;
                curColumn_ = head_;
            }
            else
            {
                curColumn_ = curColumn_->prev_;
            }
            token = strtok_noskip(NULL, ",");
        }
    }

    printf("---------------------Initial Structure----------------\n");
    curColumn_ = head_;
    while(curColumn_)
    {
        for(i = 0; i < curColumn_->bitSource_n; i++)
        {
            printf("%d ", curColumn_->bitSource[i]);
        }
        printf("\n");
        curColumn_ = curColumn_->next_;
    }
    printf("\n");

    if(!head_) return 0;

    minGenerate = generateBit;
    generateBit++;
    blocks_n = 0;
    while(maxColumn > 2)
    {

        dj_i = 0;
        while(reductionSeries[dj_i + 1] < maxColumn) dj_i++;
        dj = reductionSeries[dj_i];

        maxColumn = 0;
        curColumn_ = head_;
        while(curColumn_)
        {
            totalHeight = curColumn_->bitSource_n + curColumn_->addedSource_n;
            row = 0;
            if(totalHeight > dj)
            {
                if(!curColumn_->next_)
                {
                    temp_ = tail_;
                    tail_ = (bitColumn*) malloc(sizeof(bitColumn));
                    temp_->next_ = tail_;
                    tail_->prev_ = temp_;
                    tail_->next_ = 0;
                    tail_->bitSource_n = 0;
                    tail_->addedSource_n = 0;
                }

                while((totalHeight - dj) > 1)
                {
                    blocks[blocks_n].isFull = true;
                    blocks[blocks_n].a = curColumn_->bitSource[row];
                    blocks[blocks_n].b = curColumn_->bitSource[row + 1];
                    blocks[blocks_n].carryIn = curColumn_->bitSource[row + 2];
                    blocks[blocks_n].out = generateBit++;
                    blocks[blocks_n].carryOut = generateBit++;

                    for(i = row + 1; i < (curColumn_->bitSource_n - 2); i++)
                    {
                        curColumn_->bitSource[i] = curColumn_->bitSource[i + 2];
                    }

                    curColumn_->bitSource[row] = blocks[blocks_n].out;
                    curColumn_->next_->addedSource[curColumn_->next_->addedSource_n] = blocks[blocks_n].carryOut;

                    curColumn_->bitSource_n -= 2;
                    curColumn_->next_->addedSource_n += 1;

                    blocks_n++;
                    totalHeight -= 2;
                    row += 1;
                }

                if((totalHeight - dj) == 1)
                {
                    blocks[blocks_n].isFull = false;
                    blocks[blocks_n].a = curColumn_->bitSource[row];
                    blocks[blocks_n].b = curColumn_->bitSource[row + 1];
                    blocks[blocks_n].out = generateBit++;
                    blocks[blocks_n].carryOut = generateBit++;

                    for(i = row + 1; i < (curColumn_->bitSource_n - 1); i++)
                    {
                        curColumn_->bitSource[i] = curColumn_->bitSource[i + 1];
                    }

                    curColumn_->bitSource[row] = blocks[blocks_n].out;
                    curColumn_->next_->addedSource[curColumn_->next_->addedSource_n] = blocks[blocks_n].carryOut;

                    curColumn_->bitSource_n -= 1;
                    curColumn_->next_->addedSource_n += 1;

                    blocks_n++;
                }
            }

            for(i = (curColumn_->bitSource_n + curColumn_->addedSource_n) - 1;
                i >= curColumn_->addedSource_n;
                i--)
            {
                curColumn_->bitSource[i] = curColumn_->bitSource[i - curColumn_->addedSource_n];
            }
            for(i = 0; i < curColumn_->addedSource_n; i++){
                curColumn_->bitSource[i] = curColumn_->addedSource[i];
            }
            curColumn_->bitSource_n += curColumn_->addedSource_n;
            curColumn_->addedSource_n = 0;

            if(curColumn_->bitSource_n > maxColumn) maxColumn = curColumn_->bitSource_n;

            curColumn_ = curColumn_->next_;
        }


        blocks[blocks_n].out = -1;
        blocks_n++;
    }


    printf("---------------------Adder Blocks------------------\n");
    for(i = 0; i < blocks_n; i++)
    {
        if(blocks[i].out > -1){
            if(blocks[i].isFull == true)
            {
                printf("Block #%d:\n\tType:\tFull Adder\n\tA:\t%d%s\n\tB:\t%d%s\n\tCI:\t%d%s\n\n\tO:\t%ds\n\tCO:\t%ds\n",
                       i, blocks[i].a, (blocks[i].a > minGenerate) ? "s" : "", blocks[i].b, (blocks[i].b > minGenerate) ? "s" : "",
                       blocks[i].carryIn, (blocks[i].carryIn > minGenerate) ? "s" : "", blocks[i].out, blocks[i].carryOut);
            }
            else
            {
                printf("Block #%d:\n\tType:\tHalf Adder\n\tA:\t%d%s\n\tB:\t%d%s\n\n\tO:\t%ds\n\tCO:\t%ds\n",
                       i, blocks[i].a, (blocks[i].a > minGenerate) ? "s" : "", blocks[i].b, (blocks[i].b > minGenerate) ? "s" : "",
                       blocks[i].out, blocks[i].carryOut);
            }
        }
        else{
            printf("*********************** NEW LAYER *********************");
        }
        printf("\n");
    }

    printf("---------------------Final Stage------------------\n");
    curColumn_ = head_;
    while(curColumn_)
    {
        for(i = 0; i < curColumn_->bitSource_n; i++)
        {
            printf("%d%s ", curColumn_->bitSource[i],(curColumn_->bitSource[i] > minGenerate) ? "s" : "");
        }
        printf("\n");
        curColumn_ = curColumn_->next_;
    }
    //final adder pass



    fclose(treefile);

    return 0;
}
