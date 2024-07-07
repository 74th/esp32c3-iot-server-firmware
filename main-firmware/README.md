# ESP32-C3 IoT Server Board 用ファームウェア

- I2C Grove Port
  - [SHT31](https://www.switch-science.com/products/2853)
- GPIO Grove Port
  - [M5 IR](https://docs.m5stack.com/ja/unit/ir)

## API

### GET /ir/receive

IR リモコンの信号を受信します

#### Response

```json
{
  "lib_version": "2.8.6",
  "available": true,
  "data": {
    "type": "MITSUBISHI_AC",
    "hex": "0x23CB260100205808164000000000080000F3",
    "mes": "Power: On, Mode: 3 (Cool), Temp: 24C, Fan: 0 (Auto), Swing(V): 0 (Auto), Swing(H): 1 (Max Left), Clock: 00:00, On Timer: 00:00, Off Timer: 00:00, Timer: -, Weekly Timer: Off, 10C Heat: Off, ISee: On, Econo: Off, Absense detect: Off, Direct / Indirect Mode: 0, Fresh: Off"
  },
  "ts": 645279
}
```

### POST /ir/send

IR リモコンの信号を送信します

#### Request

```json
{
  "type": "MITSUBISHI_AC",
  "hex": "0x23CB260100205808164000000000080000F3"
}
```

#### Response

```json
{
  "ts": 1658424,
  "lib_version": "2.8.6",
  "success": true
}
```

### GET /sht31

SHT31 から温度、湿度を取得します

#### Response

```json
{
  "ts": 4647187,
  "lib_version": "1.0.0",
  "success": true,
  "data": {
    "temperature": 29.88136101,
    "humidity": 55.81292343
  }
}
```

## ファームウェアの更新方法

1. VS Code に拡張機能 [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)をインストールします
2. VS Code でこのディレクトリを開きます
3. src/ssid.hpp.sample を src/ssid.hpp にコピーし、Wi-Fi の SSID とパスワードを設定します
4. BOOT を押しながら ボードと PC を USB で接続するか、BOOT ボタンを押しながら RST ボタンを押して、書き込みモードにします
5. コマンド「PlatformIO: Upload」を押して書き込みます
