void compositePixelsCUDA(int size, quint8 *src, quint8 *dst, quint8 *mask, quint8 opacity);
void compositePixelsCUDADataTransfers(int size, quint8 *src, quint8 *dst, quint8 *mask, quint8 opacity);
void initCuda(int size);
void freeCuda();
