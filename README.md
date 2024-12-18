# **BrownSpoof**  

## **Overview**  
**BrownSpoof** is an embedded system that emulates the magnetic stripe signal of a **Brown University ID card**. Using an **H-bridge**, **coil**, and **Arduino microcontroller**, it generates a magnetic signal that can be read by standard card readers used on campus. The system allows for **real-time updates to Track 2 data** via **Wi-Fi**, enabling dynamic card data changes without manual reprogramming.

This project draws inspiration from **[Samy Kamkar's MagSpoof](https://github.com/samyk/magspoof)** and builds on its concepts with support for **Wi-Fi connectivity**, **dynamic data updates**, and **LED matrix status feedback**.  

---

## **How It Works**  
1. **Power On & Wi-Fi Connection**  
   - The system powers on and connects to the **Wi-Fi network**.  
   - If successful, a **checkmark animation** appears on the LED matrix. If unsuccessful, a **bug animation** is displayed.  

2. **Track 2 Data Updates**  
   - Press the **server button** to send a request to a server for updated Track 2 data.  
   - The system fetches and stores the new data for use during signal playback.  

3. **Signal Playback**  
   - Press the **play button** to generate a magnetic signal for the stored Track 2 data.  
   - The signal follows the **ISO/IEC 7811-2 standard**, ensuring compatibility with card readers.  

---

## **How to Use**  
1. **Setup**  
   - Connect the H-bridge, coil, LED matrix, and buttons to the Arduino.  
   - Upload the provided **MagSpoof.ino** file to your Arduino board.  

2. **Run the System**  
   - Power on the device. It will attempt to connect to Wi-Fi.  
   - Press the **server button** to fetch new Track 2 data from the server.  
   - Press the **play button** to emit the magnetic signal for the stored data.  

---

## **Contributors**  
- **Anoop Kiran**  
- **Alex Lin**  
- **Kaiyang Yao**  
- **Korgan Atillasoy**  

---

## **Credits**  
This project is inspired by **[Samy Kamkar's MagSpoof](https://github.com/samyk/magspoof)**, a well-known project for magnetic stripe spoofing. The concepts of **signal generation and H-bridge polarity control** were adapted and customized for broader use cases.  

---

If you'd like to contribute or have any questions, feel free to submit an issue or pull request.  
🎉 **Happy Spoofing!** 🎉
