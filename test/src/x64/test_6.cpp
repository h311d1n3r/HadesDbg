int main() {
    unsigned int i = 496;
    unsigned int* i_ptr = &i;
    i_ptr += 5;
    unsigned int** i_ptr_ptr = &i_ptr;
    i_ptr_ptr -= 1745;
    unsigned int a = 0;
    a+=1;
    a-=3;
    return 0;
}
