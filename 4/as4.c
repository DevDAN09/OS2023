#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define NUM_ADDRESSES 5000

/* 가상 주소 정보 */
typedef struct{
    int virtualAddress;
    int pageNum;
} VirtualAddressInfo;

/* 페이지 테이블 */
typedef struct{
    int pageNum;
    int frameNum;
    int referenceBit;
} PageTable;

/* 물리 메모리 */
typedef struct{
    int isOccupied;
    int frameNum;
    int physicalAddress;
} PhysicalMemoryInfo;

typedef struct {
    int *items;
    int front;
    int rear;
    int maxSize;
} PageQueue;

void initializePageQueue(PageQueue *q, int maxSize) {
    q->items = (int*)malloc(maxSize * sizeof(int));
    if (q->items == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    q->maxSize = maxSize;
    q->front = -1;
    q->rear = -1;
}

int isFull(PageQueue *q) {
    if ((q->rear + 1) % q->maxSize == q->front) {
        return 1;
    } else {
        return 0;
    }
}

int isEmpty(PageQueue *q) {
    if (q->front == -1) {
        return 1;
    } else {
        return 0;
    }
}

int enqueue(PageQueue *q, int value) {
    if (isFull(q)) {
        printf("Queue is full.\n");
        return -1;
    } else {
        if (q->front == -1) {
            q->front = 0;
        }
        q->rear = (q->rear + 1) % q->maxSize;
        q->items[q->rear] = value;
        return 0;
    }
}

int dequeue(PageQueue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty.\n");
        return -1;
    } else {
        int item = q->items[q->front];
        if (q->front == q->rear) {
            // Queue has only one element
            initializePageQueue(q, q->maxSize);
        } else {
            q->front = (q->front + 1) % q->maxSize;
        }
        return item;
    }
}

int findPositionInQueue(PageQueue *q, int value) {
    // 큐가 비어있는지 확인
    if (q->front == -1) {
        return -1;
    }

    int pos = -1;
    for (int i = q->front; ; i = (i + 1) % q->maxSize) {
        if (q->items[i] == value) {
            pos = i;
            break;
        }
        if (i == q->rear) break; // 모든 요소를 검사한 경우 루프 종료
    }
    return pos;
}

void removeFrameFromQueue(PageQueue *q, int value) {
    int pos = findPositionInQueue(q, value);
    if (pos == -1){
        printf("프레임 값: %d 에 대한 위치가 존재하지 않습니다.\n", value);
        return;
    }  // 값이 큐에 없으면 함수 종료

    // pos 위치부터 큐의 요소들을 앞으로 이동
    for (int i = pos; i != q->rear; i = (i + 1) % q->maxSize) {
        int next = (i + 1) % q->maxSize;
        q->items[i] = q->items[next];
    }
    q->items[q->rear] = -1;
    // rear 포인터 감소
    q->rear = (q->rear - 1 + q->maxSize) % q->maxSize;
    if (q->rear == q->front) {
        // 큐가 비었을 경우
        q->front = -1;
        q->rear = -1;
    }
}

void freeQueue(PageQueue *q) {
    free(q->items);
}

// 파일로 부터 가상 주소를 읽어오는 함수
int readVirtualAddress(FILE *file, VirtualAddressInfo* virtualAddressInfos, int *count) {
    int readData;
    while (*count < NUM_ADDRESSES) {
        if (fscanf(file, "%d", &virtualAddressInfos[*count].virtualAddress) == EOF) {
            return 0; // 파일의 끝에 도달하거나 읽기 실패
        }
        (*count)++;
        return 1; // 성공적으로 주소를 읽음
    }
    return 0;
}

VirtualAddressInfo* createVirtualAddressInfoArray(){
    VirtualAddressInfo* table = (VirtualAddressInfo*)malloc(sizeof(VirtualAddressInfo) * NUM_ADDRESSES);
    for (int i = 0; i < NUM_ADDRESSES; i++) {
        table[i].pageNum = -1;
        table[i].virtualAddress = -1;      // 초기 유효 비트는 0 (유효하지 않음)
    }
    return table;
}

PageTable* createPageTable(int numPages){
    PageTable* table = (PageTable*)malloc(sizeof(PageTable) * numPages);
    for (int i = 0; i < numPages; i++) {
        table[i].pageNum = -1;
        table[i].frameNum = -1; // 초기 프레임 번호는 0
        table[i].referenceBit = 0;       // 초기 유효 비트는 0 (유효하지 않음)
    }
    return table;
}

PhysicalMemoryInfo* createPhysicalMemoryInfoArray(int numFrames){
    PhysicalMemoryInfo* table = (PhysicalMemoryInfo*)malloc(sizeof(PhysicalMemoryInfo) * numFrames);
    for (int i = 0; i < numFrames; i++) {
        table[i].frameNum = -1;
        table[i].physicalAddress = -1; // 초기 프레임 번호는 0
        table[i].isOccupied = 0;       // 초기 유효 비트는 0 (유효하지 않음)
    }
    return table;
}

int getPhysicalAddressFromVA(int virtualAddress, PageTable pageTable[], PhysicalMemoryInfo frames[], int totalPage, int totalFrame, int pageSizeByte){
    int tmpPageNum = virtualAddress / pageSizeByte;
    int matchedPageNum = -1;
    for(int i = 0; i < totalPage; i++){
        if(pageTable[i].pageNum == tmpPageNum){
            matchedPageNum = i;
        }
    }

    for(int i = 0; i < totalFrame; i++){
        if(pageTable[matchedPageNum].frameNum == frames[i].frameNum){
            return frames[i].physicalAddress;
        }
    }
}

int getFrameNumFromVA(int virtualAddress, PageTable* pageTable, int totalPage, int pageSizeKB){
    int tmpPageNum = virtualAddress / pageSizeKB;
    for(int i = 0; i < totalPage; i++){
        if(pageTable[i].pageNum == tmpPageNum){
            return pageTable[i].frameNum;
        }
    }
}

int replacePageOptimal(PhysicalMemoryInfo frames[], VirtualAddressInfo virtualAddressInfoArray[], PageTable pageTable[], PageQueue* pageQueue,int currentIndex, int totalFrame, int totalPage, int pageSizeByte){
    int occupiedCount = 0;
    int offset = virtualAddressInfoArray[currentIndex].virtualAddress % pageSizeByte;
    int pageNum = virtualAddressInfoArray[currentIndex].virtualAddress / pageSizeByte;
    int frameNumfromPageTable = pageTable[pageNum].frameNum;
    
    // Page hit 확인
    for(int i = 0; i < totalFrame ; i++){
        occupiedCount += 1; // page hit가 발생하지 않으면 해당 카운트는 totalFrame과 같게됨

        int pageNumFromFrame = -1;
        for(int j = 0; j < totalPage; j++){
            if(pageTable[j].frameNum == frames[i].frameNum){
                pageNumFromFrame = pageTable[j].pageNum;
            }
        }

        if(frames[i].isOccupied && virtualAddressInfoArray[currentIndex].pageNum == pageNumFromFrame){
            // this is hit
            frames[i].isOccupied = 1;
            frames[i].physicalAddress = i * pageSizeByte + offset;
            return 1; // pagehit를 반환
        }
    }

    // Hit가 아니라면 
    // 1. 초기화 상황
    for(int i = 0 ; i < totalFrame ; i++){
        if(!frames[i].isOccupied){
            // 해당 프레임에 초기화를 진행
            virtualAddressInfoArray[currentIndex].pageNum = pageNum;
            pageTable[virtualAddressInfoArray[currentIndex].pageNum].pageNum = pageNum;
            pageTable[virtualAddressInfoArray[currentIndex].pageNum].frameNum = i;
            frames[pageTable[pageNum].frameNum].isOccupied = 1;
            frames[pageTable[pageNum].frameNum].frameNum = i;
            frames[pageTable[pageNum].frameNum].physicalAddress = i * pageSizeByte  + offset;
            enqueue(pageQueue, frames[pageTable[pageNum].frameNum].frameNum);

            return 0; // pageFault 반환
        }
    }

    // 2. page 폴트 상황
    int distances[totalFrame];
    for(int i = 0; i < totalFrame; i++){
        distances[i] = 0;
    }
    int maxDistanceFrame = -1;
    int maxDistanceFrameNum = 0;
    int notAppearCount = 0;
    
    if(occupiedCount == totalFrame){ //모든 페이지가 occupied 되어 있다면
        for(int i = 0; i < totalFrame ; i++){ // 프레임 전체에 대해서
            int frameNumforSeach = frames[i].frameNum; // i 번째 프레임의 페이지 넘버를 탐색
            int pageNumforSearch = 0;
            
            for(int j = 0; j < totalPage ; j++){
                if(frameNumforSeach == pageTable[j].frameNum){
                    pageNumforSearch = pageTable[j].pageNum;
                    break;
                }
            }

            for(int j = currentIndex; j < NUM_ADDRESSES; j++){ 
                if(virtualAddressInfoArray[j].pageNum == pageNumforSearch){ 
                    distances[i] = j - currentIndex;
                    //printf("distance[%d] = %d\n",i, distances[i]);
                    break;
                }
                else if(j == NUM_ADDRESSES-1){
                    //printf("뒤에 다시 나타나지 않음\n");
                    distances[i] = 5000;
                    notAppearCount++;
                }
            }

            if(maxDistanceFrame < distances[i]){
                maxDistanceFrame = distances[i];
                maxDistanceFrameNum = i;
            }
        }

        int firstOutFrameNum = -1;
        int pos = -1;
        int firstOutPos = -1;

        if(notAppearCount > 1){ //더 이상 나타나지 않는 페이지 번호가 2개 이상 존재
                /* victim Page 후보들중 fifo를 수행*/
                //1. victim pages 중에 먼저 queue가 진행되는 값을 뽑는다
                //printf("current : %d %d\n", currentIndex, virtualAddressInfoArray[currentIndex].virtualAddress);
                for(int k = 0; k < totalFrame; k++){
                    //printf("distance[%d] : %d\n", k, distances[k]);
                    if(distances[k] == 5000){
                        pos = findPositionInQueue(pageQueue, k);
                        if(pos == -1){ //
                            //해당 프레임은 현재 큐에 없음
                        } else { // 해당 프레임은 현재 큐에 존재함
                            if(firstOutPos == -1){ // 초기화
                                firstOutPos = pos;
                                firstOutFrameNum = k;
                            }
                        
                            if(pos < firstOutPos){ // 선입 선출
                                firstOutPos = pos;
                                firstOutFrameNum = k;
                            }
                        }
                        //printf("current k-> %d, pos : %d\n", k, pos);



                    }
                }

            maxDistanceFrameNum = firstOutFrameNum;
        }


        virtualAddressInfoArray[currentIndex].pageNum = pageNum;
        pageTable[virtualAddressInfoArray[currentIndex].pageNum].pageNum = pageNum;
        pageTable[virtualAddressInfoArray[currentIndex].pageNum].frameNum = maxDistanceFrameNum;
        frames[maxDistanceFrameNum].isOccupied = 1;
        frames[maxDistanceFrameNum].frameNum = maxDistanceFrameNum;
        frames[maxDistanceFrameNum].physicalAddress = maxDistanceFrameNum * pageSizeByte + offset;
        // 이후에 제일 늦게 사용될 되는 프레임을 큐에서 제거
        removeFrameFromQueue(pageQueue, maxDistanceFrameNum);
        // 교체가 진행된 페이지를 enqueue 진행
        if(enqueue(pageQueue, maxDistanceFrameNum) == -1){
            //printf("current Index : %d\n\n", currentIndex);
        }

        for(int i = 0; i < totalPage; i++){ // 기존 pageTable 에 있는 정보 초기화
            if(frames[maxDistanceFrameNum].frameNum == pageTable[i].frameNum && pageNum != pageTable[i].pageNum ){
                pageTable[i].frameNum = -1;
                pageTable[i].pageNum = -1;
            }
        }
        return 0; //pageFault 반환
    } 
}

int replacePageSC(PhysicalMemoryInfo frames[], VirtualAddressInfo virtualAddressInfoArray[], PageTable pageTable[], PageQueue* pageQueue,int currentIndex, int totalFrame, int totalPage, int pageSizeByte){
    int occupiedCount = 0;
    int offset = virtualAddressInfoArray[currentIndex].virtualAddress % pageSizeByte;
    int pageNum = virtualAddressInfoArray[currentIndex].virtualAddress / pageSizeByte;
    int frameNumfromPageTable = pageTable[pageNum].frameNum;
    
    // Page hit 확인
    for(int i = 0; i < totalFrame ; i++){
        occupiedCount += 1; // page hit가 발생하지 않으면 해당 카운트는 totalFrame과 같게됨

        int pageNumFromFrame = -1;
        for(int j = 0; j < totalPage; j++){
            if(pageTable[j].frameNum == frames[i].frameNum){
                pageNumFromFrame = pageTable[j].pageNum;
                //break;
            }
        }

        if(frames[i].isOccupied && virtualAddressInfoArray[currentIndex].pageNum == pageNumFromFrame){
            // this is hit
            frames[i].isOccupied = 1;
            frames[i].physicalAddress = i * pageSizeByte + offset;

            // 참조 되었으므로 1로 올림
            pageTable[pageNumFromFrame].referenceBit = 1;
            return 1; // pagehit를 반환
        }
    }

    // Hit가 아니라면 
    // 1. 초기화 상황
    for(int i = 0 ; i < totalFrame ; i++){
        if(!frames[i].isOccupied){
            // 해당 프레임에 초기화를 진행
            virtualAddressInfoArray[currentIndex].pageNum = pageNum;
            pageTable[virtualAddressInfoArray[currentIndex].pageNum].pageNum = pageNum;
            pageTable[virtualAddressInfoArray[currentIndex].pageNum].frameNum = i;
            frames[pageTable[pageNum].frameNum].isOccupied = 1;
            frames[pageTable[pageNum].frameNum].frameNum = i;
            frames[pageTable[pageNum].frameNum].physicalAddress = i * pageSizeByte  + offset;

            pageTable[virtualAddressInfoArray[currentIndex].pageNum].referenceBit = 0;
            enqueue(pageQueue, frames[pageTable[pageNum].frameNum].frameNum);
            return 0; // pageFault 반환
        }
    }

    // 2. page 폴트 상황
    if(occupiedCount == totalFrame){ //모든 페이지가 occupied 되어 있다면
        int victimFrameNum = dequeue(pageQueue);
        int victimPageNum = -1;

        /* victim Frame 찾기 */

        for(int j = 0; j < totalPage ; j++){ // dequeue를 통해 받은 피해자 프레임의 페이지 넘버를 찾는다.
            if(victimFrameNum == pageTable[j].frameNum){
                victimPageNum = pageTable[j].pageNum;
                if(pageTable[victimPageNum].referenceBit == 1){
                    pageTable[victimPageNum].referenceBit = 0; // referenceBit 를 0
                    enqueue(pageQueue, victimFrameNum); // 기회를 다시 줌
                    victimFrameNum = dequeue(pageQueue); // 새로운 victim 프레임선택
                    j = -1; // 새로운 피해자를 가지고 다시 탐색
                }
                else{ // referenceBit가 0 일때
                    break;
                }
            }
        }

        /* 이전 페이지 테이블 내용 삭제*/
        for(int j = 0; j < totalPage; j++){
            if(victimFrameNum == pageTable[j].frameNum && pageNum != pageTable[j].pageNum){
                pageTable[j].frameNum = -1;
                pageTable[j].pageNum = -1;
                
            }
        }

        virtualAddressInfoArray[currentIndex].pageNum = pageNum;
        pageTable[virtualAddressInfoArray[currentIndex].pageNum].pageNum = pageNum;
        pageTable[virtualAddressInfoArray[currentIndex].pageNum].frameNum = victimFrameNum;
        pageTable[virtualAddressInfoArray[currentIndex].pageNum].referenceBit = 0;
        frames[victimFrameNum].isOccupied = 1;
        frames[victimFrameNum].frameNum = victimFrameNum;
        frames[victimFrameNum].physicalAddress = victimFrameNum * pageSizeByte + offset;

        enqueue(pageQueue, victimFrameNum);

        /*
             for(int i = 0; i < totalPage; i++){ // 기존 pageTable 에 있는 정보 초기화
            if(frames[maxDistanceFrameNum].frameNum == pageTable[i].frameNum && pageNum != pageTable[i].pageNum ){
                pageTable[i].frameNum = -1;
                pageTable[i].pageNum = -1;
            }
        }
        */
   
        

        return 0; //pageFault 반환
    } 
}

int replacePageLRU(PhysicalMemoryInfo frames[], VirtualAddressInfo virtualAddressInfoArray[], PageTable pageTable[],int currentIndex, int totalFrame, int totalPage, int pageSizeByte){
    int occupiedCount = 0;
    int offset = virtualAddressInfoArray[currentIndex].virtualAddress % pageSizeByte;
    int pageNum = virtualAddressInfoArray[currentIndex].virtualAddress / pageSizeByte;
    int frameNumfromPageTable = pageTable[pageNum].frameNum;
    
    // Page hit 확인
    
    for(int i = 0; i < totalFrame ; i++){
        occupiedCount += 1; // page hit가 발생하지 않으면 해당 카운트는 totalFrame과 같게됨

        int pageNumFromFrame = -1;
        for(int j = 0; j < totalPage; j++){
            if(pageTable[j].frameNum == frames[i].frameNum){
                pageNumFromFrame = pageTable[j].pageNum;
            }
        }

        if(frames[i].isOccupied && virtualAddressInfoArray[currentIndex].pageNum == pageNumFromFrame){
            // this is hit
            frames[i].isOccupied = 1;
            frames[i].physicalAddress = i * pageSizeByte + offset;
            return 1; // pagehit를 반환
        }
    }

    // Hit가 아니라면 
    // 1. 초기화 상황
    for(int i = 0 ; i < totalFrame ; i++){
        if(!frames[i].isOccupied){
            // 해당 프레임에 초기화를 진행
            virtualAddressInfoArray[currentIndex].pageNum = pageNum;
            pageTable[virtualAddressInfoArray[currentIndex].pageNum].pageNum = pageNum;
            pageTable[virtualAddressInfoArray[currentIndex].pageNum].frameNum = i;
            frames[pageTable[pageNum].frameNum].isOccupied = 1;
            frames[pageTable[pageNum].frameNum].frameNum = i;
            frames[pageTable[pageNum].frameNum].physicalAddress = i * pageSizeByte  + offset;
            return 0; // pageFault 반환
        }
    }

    // 2. page 폴트 상황
    
    int distances[totalFrame];
    for(int i = 0; i < totalFrame; i++){
        distances[i] = 0;
    }
    int maxDistanceFrame = -1;
    int maxDistanceFrameNum = 0;
    
    if(occupiedCount == totalFrame){ //모든 페이지가 occupied 되어 있다면
        for(int i = 0; i < totalFrame ; i++){ // 프레임 전체에 대해서
            int frameNumforSeach = frames[i].frameNum; // i 번째 프레임의 페이지 넘버를 탐색
            int pageNumforSearch = 0;
            
            for(int j = 0; j < totalPage ; j++){
                if(frameNumforSeach == pageTable[j].frameNum){
                    pageNumforSearch = pageTable[j].pageNum;
                    break;
                }
            }

            for(int j = currentIndex; -1 < j ; j--){ 
                if(virtualAddressInfoArray[j].pageNum == pageNumforSearch){ 
                    distances[i] = currentIndex - j;
                    //printf("distance[%d] = %d\n",i, distances[i]);
                    break;
                }
                else if(j == 4999){
                    //printf("뒤에 다시 나타나지 않음\n");
                    distances[i] = 5000;
                }
            }

            // 가장 최근에 사용되지 않은 프레임을 교체한다
            if(maxDistanceFrame < distances[i]){
                maxDistanceFrame = distances[i];
                maxDistanceFrameNum = i;
            }
            
        }

        virtualAddressInfoArray[currentIndex].pageNum = pageNum;
        pageTable[virtualAddressInfoArray[currentIndex].pageNum].pageNum = pageNum;
        pageTable[virtualAddressInfoArray[currentIndex].pageNum].frameNum = maxDistanceFrameNum;
        frames[maxDistanceFrameNum].isOccupied = 1;
        frames[maxDistanceFrameNum].frameNum = maxDistanceFrameNum;
        frames[maxDistanceFrameNum].physicalAddress = maxDistanceFrameNum * pageSizeByte + offset;

        for(int i = 0; i < totalPage; i++){ // 기존 pageTable 에 있는 정보 초기화
            if(frames[maxDistanceFrameNum].frameNum == pageTable[i].frameNum && pageNum != pageTable[i].pageNum ){
                pageTable[i].frameNum = -1;
                pageTable[i].pageNum = -1;
            }
        }
        return 0; //pageFault 반환
    } 
}

int replacePageFIFO(PhysicalMemoryInfo frames[], VirtualAddressInfo virtualAddressInfoArray[], PageTable pageTable[], PageQueue* pageQueue,int currentIndex, int totalFrame, int totalPage, int pageSizeByte){
    int occupiedCount = 0;
    int offset = virtualAddressInfoArray[currentIndex].virtualAddress % pageSizeByte;
    int pageNum = virtualAddressInfoArray[currentIndex].virtualAddress / pageSizeByte;
    int frameNumfromPageTable = pageTable[pageNum].frameNum;
    
    // Page hit 확인
    for(int i = 0; i < totalFrame ; i++){
        occupiedCount += 1; // page hit가 발생하지 않으면 해당 카운트는 totalFrame과 같게됨

        int pageNumFromFrame = -1;
        for(int j = 0; j < totalPage; j++){
            if(pageTable[j].frameNum == frames[i].frameNum){
                pageNumFromFrame = pageTable[j].pageNum;
            }
        }

        if(frames[i].isOccupied && virtualAddressInfoArray[currentIndex].pageNum == pageNumFromFrame){
            // this is hit
            frames[i].isOccupied = 1;
            frames[i].physicalAddress = i * pageSizeByte + offset;
            return 1; // pagehit를 반환
        }
    }

    // Hit가 아니라면 
    // 1. 초기화 상황
    for(int i = 0 ; i < totalFrame ; i++){
        if(!frames[i].isOccupied){
            // 해당 프레임에 초기화를 진행
            virtualAddressInfoArray[currentIndex].pageNum = pageNum;
            pageTable[virtualAddressInfoArray[currentIndex].pageNum].pageNum = pageNum;
            pageTable[virtualAddressInfoArray[currentIndex].pageNum].frameNum = i;
            frames[pageTable[pageNum].frameNum].isOccupied = 1;
            frames[pageTable[pageNum].frameNum].frameNum = i;
            frames[pageTable[pageNum].frameNum].physicalAddress = i * pageSizeByte  + offset;
            enqueue(pageQueue, frames[pageTable[pageNum].frameNum].frameNum);
            return 0; // pageFault 반환
        }
    }

    // 2. page 폴트 상황
    if(occupiedCount == totalFrame){ //모든 페이지가 occupied 되어 있다면

        int victimFrameNum = dequeue(pageQueue);
        int victimPageNum = -1;

        for(int j = 0; j < totalPage ; j++){ // dequeue를 통해 받은 피해자 프레임의 페이지 넘버를 찾는다.
            if(victimFrameNum == pageTable[j].frameNum && pageTable[j].pageNum != pageNum){
                victimPageNum = pageTable[j].pageNum;
                /* 이전 페이지 테이블 초기화*/
                pageTable[victimPageNum].frameNum = -1;
                pageTable[victimPageNum].pageNum = -1;
                //break;
            }
        }

        virtualAddressInfoArray[currentIndex].pageNum = pageNum;
        pageTable[virtualAddressInfoArray[currentIndex].pageNum].pageNum = pageNum;
        pageTable[virtualAddressInfoArray[currentIndex].pageNum].frameNum = victimFrameNum;
        frames[victimFrameNum].isOccupied = 1;
        frames[victimFrameNum].frameNum = victimFrameNum;
        frames[victimFrameNum].physicalAddress = victimFrameNum * pageSizeByte + offset;

        enqueue(pageQueue, victimFrameNum);

        /*
             for(int i = 0; i < totalPage; i++){ // 기존 pageTable 에 있는 정보 초기화
            if(frames[maxDistanceFrameNum].frameNum == pageTable[i].frameNum && pageNum != pageTable[i].pageNum ){
                pageTable[i].frameNum = -1;
                pageTable[i].pageNum = -1;
            }
        }
        */
   
        

        return 0; //pageFault 반환
    } 
}

int main(){
    int virtualAddressBits, pageSize, physicalMemorySize;
    int pageReplacementAlgorithm, fileOption;
    int pageSizeBytes;
    VirtualAddressInfo* virtualAddressInfoArray = createVirtualAddressInfoArray();
    PageTable* pageTable;
    PhysicalMemoryInfo* physicalMemoryInfoArray;
    PageQueue pageQueue;

    FILE *file, *outputFile;
    char fileName[256];
    
    int totalVirtualMemory;
    int totalPage;
    int totalFrame;

    int pageFaults = 0, pageHits = 0, result =0;
    char pageFaultChar = ' ';


    // 가상 주소의 길이 선택
    printf("A. Simulation에 사용할 가상주소 길이를 선택하시오 (1. 18bits 2. 19bits 3. 20bits): ");
    scanf("%d", &virtualAddressBits);
    while (virtualAddressBits < 1 || virtualAddressBits > 3) {
        printf("유효하지 않은 선택입니다. 다시 선택해주세요: ");
        scanf("%d", &virtualAddressBits);
    }    if(virtualAddressBits == 1){
        virtualAddressBits = 18;
    }
    else if(virtualAddressBits == 2 ){
        virtualAddressBits = 19;
    }
    else{
        virtualAddressBits = 20;
    }

    /* 가상주소의 최대 값*/
    totalVirtualMemory = 1 << virtualAddressBits;

    // 페이지 크기 선택
    printf("\nB. Simulation에 사용할 페이지(프레임)의 크기를 선택하시오 (1. 1KB 2. 2KB 3. 4KB): ");
    scanf("%d", &pageSize);
    while (pageSize < 1 || pageSize > 3) {
        printf("유효하지 않은 선택입니다. 다시 선택해주세요: ");
        scanf("%d", &pageSize);
    }
    if(pageSize == 1){
        pageSize = 1;
    }
    else if(pageSize == 2){
        pageSize = 2;
    }
    else{
        pageSize = 4;
    }

    /* 총 페이지 사이즈 */
    pageSizeBytes = pageSize * 1024;
    totalPage = totalVirtualMemory / pageSizeBytes;
    /* 페이지 테이블 초기화 */
    pageTable = createPageTable(totalPage);
    

    // 물리 메모리 크기 선택
    printf("\nC. Simulation에 사용할 물리메모리의 크기를 선택하시오 (1. 32KB 2. 64KB): ");
    scanf("%d", &physicalMemorySize);
    while (physicalMemorySize < 1 || physicalMemorySize > 2) {
        printf("유효하지 않은 선택입니다. 다시 선택해주세요: ");
        scanf("%d", &physicalMemorySize);
    }
    if(physicalMemorySize == 1){
        physicalMemorySize = 32;
    }
    else{
        physicalMemorySize = 64;
    }

    totalFrame = physicalMemorySize / pageSize;
    /* 물리메모리 초기화 */
    physicalMemoryInfoArray = createPhysicalMemoryInfoArray(totalFrame);

    /* 큐 초기화 */
    initializePageQueue(&pageQueue, totalFrame);

    // 페이지 교체 알고리즘 선택
    printf("\nD. Simulation에 적용할 Page Replacement 알고리즘을 선택하시오 \n\
    (1. Optimal 2. FIFO 3. LRU 4. Second-Chance): ");
    scanf("%d", &pageReplacementAlgorithm);
    while (pageReplacementAlgorithm < 1 || pageReplacementAlgorithm > 4) {
        printf("유효하지 않은 선택입니다. 다시 선택해주세요: ");
        scanf("%d", &pageReplacementAlgorithm);
    }

    // 가상 주소 스트링 입력 방식 선택
    printf("\nE. 가상주소 스트링 입력방식을 선택하시오\n\
    (1. input.in 자동 생성, 2. 기존 파일 사용): ");
    scanf("%d", &fileOption);
    while (fileOption < 1 || fileOption > 2) {
        printf("유효하지 않은 선택입니다. 다시 선택해주세요: ");
        scanf("%d", &fileOption);
    }

    if (fileOption == 1) {
        // input.in 파일 생성 및 열기
  
        file = fopen("input.in", "w");
        if (file == NULL) {
            fprintf(stderr, "파일을 열 수 없습니다.\n");
            exit(1);
        }

        // 임의의 가상 주소 생성 및 파일에 작성
        srand(time(NULL)); // 난수 초기화
        for (int i = 0; i < NUM_ADDRESSES; i++) {
            unsigned long maxAddress = 1UL << virtualAddressBits; // 최대 주소 계산
            unsigned long address = rand() % maxAddress; // 임의의 주소 생성
            fprintf(file, "%lu\n", address);
        }

        // 파일 닫기
        fclose(file);
        //printf("input.in 파일이 생성되었습니다.\n");
        file = fopen("input.in", "r");
        if (file == NULL) {
            fprintf(stderr, "파일을 열 수 없습니다.\n");
            exit(1);
        }   
        
    } else{
        printf("\nF. 입력 파일 이름을 입력하시오: ");
        scanf("%s", fileName);
        // 파일 열기
        file = fopen(fileName, "r");
        if (file == NULL) {
            fprintf(stderr, "파일을 열 수 없습니다.\n");
            exit(1);
        }   
    }

    /* input.in 에서 가상 주소 불러오기 */
    int currentVACount = 0;
    while (readVirtualAddress(file, virtualAddressInfoArray, &currentVACount)) {
        int virtualAddress = virtualAddressInfoArray[currentVACount-1].virtualAddress;
        int maxAddress = (1 << virtualAddressBits) - 1;

        if (virtualAddress < 0 || virtualAddress > maxAddress) {
            printf("입력된 가상주소의 길이가 벗어난 가상주소가 발견되었습니다. 프로그램을 종료합니다.\n");
            exit(1);
        } 
    }

    

    // input.in 파일 닫기
    fclose(file);


    //printf("No.%7sV.A.%10sPage No.%6sFrame No.%6sP.A.%10sPage Fault\n", "","","","","");
    
    /* outputfile 열기 */
    switch(pageReplacementAlgorithm){
        case 1:
            outputFile = fopen("output.opt", "w");
            break;
        case 2:
            outputFile = fopen("output.fifo", "w");
            break;
        case 3:
            outputFile = fopen("output.lru", "w");
            break;
        case 4:
            outputFile = fopen("output.sc", "w");
            break;
        default:
            printf("error");
            exit(1);
    }
         
    fprintf(outputFile, "%-10s%-10s%-10s%-10s%-10sPage Fault\n", "No.","V.A.","Page No.","Frame No.","P.A.");

    for(int i = 0; i < NUM_ADDRESSES; i++){
        virtualAddressInfoArray[i].pageNum = virtualAddressInfoArray[i].virtualAddress / pageSizeBytes;
    }

    for(int i = 0; i < NUM_ADDRESSES; i++){
        
        // 교체 알고리즘에 맞춰서 처리
        if(pageReplacementAlgorithm == 1){
            //printf("Optimal Algorithmn Start\n");
            result = replacePageOptimal(physicalMemoryInfoArray, virtualAddressInfoArray, pageTable, &pageQueue, i, totalFrame, totalPage, pageSizeBytes);
            //printf("pageTable[%d].pageNum : %d\n", virtualAddressInfoArray[i].pageNum, pageTable[virtualAddressInfoArray[i].pageNum].pageNum);
            //printf("Frames[%d].frameNum : %d\n", pageTable[virtualAddressInfoArray[i].pageNum].frameNum, physicalMemoryInfoArray[pageTable[virtualAddressInfoArray[i].pageNum].pageNum].frameNum);
            if(result){
                pageHits++;
                pageFaultChar = 'H';
            } else {
                pageFaults++;
                pageFaultChar = 'F';
            }
 
        }
        else if(pageReplacementAlgorithm == 2){
            
            
            result = replacePageFIFO(physicalMemoryInfoArray, virtualAddressInfoArray, pageTable, &pageQueue, i, totalFrame, totalPage, pageSizeBytes);
            if(result){
                pageHits++;
                pageFaultChar = 'H';
            } else{
                pageFaults++;
                pageFaultChar = 'F';
            }
                
        }
        else if(pageReplacementAlgorithm == 3){
            result = replacePageLRU(physicalMemoryInfoArray, virtualAddressInfoArray, pageTable, i, totalFrame, totalPage, pageSizeBytes);
            if(result){
                pageHits++;
                pageFaultChar = 'H';
            } else{
                pageFaults++;
                pageFaultChar = 'F';
            }
        }
        else if(pageReplacementAlgorithm == 4){
            result = replacePageSC(physicalMemoryInfoArray, virtualAddressInfoArray, pageTable, &pageQueue, i, totalFrame, totalPage, pageSizeBytes);
            if(result){
                pageHits++;
            pageFaultChar = 'H';
            } else{
                pageFaults++;
                pageFaultChar = 'F';
            }
        }

        fprintf(outputFile, "%-10d",i+1);
        fprintf(outputFile, "%-10d", virtualAddressInfoArray[i].virtualAddress);
        fprintf(outputFile, "%-10d", virtualAddressInfoArray[i].pageNum);
        fprintf(outputFile, "%-10d", getFrameNumFromVA(virtualAddressInfoArray[i].virtualAddress, pageTable, totalPage, pageSizeBytes));
        fprintf(outputFile, "%-10d", getPhysicalAddressFromVA(virtualAddressInfoArray[i].virtualAddress, pageTable, physicalMemoryInfoArray, totalPage, totalFrame, pageSizeBytes));
        fprintf(outputFile, "%-10c\n", pageFaultChar);

        if(i == 4999){
            fprintf(outputFile, "Total Number of Page Faults: %d\n", pageFaults);
        }

       
    }
    freeQueue(&pageQueue);
    fclose(outputFile);
    return 0;
}

//Confirm
