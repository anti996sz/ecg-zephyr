int fs_init(void);

//save ECG data as integer with µV unit
void save_data(int16_t data[], uint8_t length, uint64_t counter);

void sync_data(void);