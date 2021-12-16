# ecg-zephyr
ECG device based on zephyr, ads1298r used.

//电压转换 in app
  double transAmp(int ecgData) {
    if (BleTool.oneLead()) {
      //单导设备
      return (3300 - 0) *
          ecgData.toDouble() /
          (((1 << 12) - 1) * 400) *
          8; //电压伏转毫伏
    } else {
      //7导及以上设备
      return (2400 - 0) * ecgData.toDouble() / (((1 << 23) - 1) * 12); //电压伏转毫伏
    }
  }
