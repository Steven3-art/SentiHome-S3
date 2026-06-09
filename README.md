\# 🏠 SentiHome S3 — Autonomous Smart Home AI System



> Built in Yaoundé, Cameroon 🇨🇲 for Google Cloud Multi-Agent Hackathon 2026



\## 🎯 Overview

SentiHome S3 is an autonomous smart home system powered by ESP32 S3 

and 7 specialized Gemini AI agents, each with unique personality and role.



\## 🤖 7 AI Agents

| Agent | Role | Sensor |

|-------|------|--------|

| AURA | Climate Guardian | DHT22 |

| LUX | Light Controller | LDR + 3 Relays |

| RADAR | Presence Detection | HLK-LD2410 |

| PURETÉ | Air Quality Doctor | Sharp GP2Y10 |

| VOLT | Energy Monitor | Voltage Divider |

| CONNECT | Emergency SMS | GSM A7 |

| VISION | Surveillance | Servo + ESP32-CAM |



\## ☁️ Google Cloud Stack

\- \*\*Vertex AI Agent Platform\*\* (Gemini 2.5 Flash)

\- \*\*Firebase Hosting\*\* + Realtime Database

\- \*\*Cloud Run\*\* (MCP Server + REST API)

\- \*\*Google ADK\*\* (Agent Development Kit)



\## 🔗 Links

\- \*\*Live Demo:\*\* https://gen-lang-client-0279809768.web.app

\- \*\*Cloud Run API:\*\* https://sentihome-api-968643109684.us-central1.run.app



\## 🚀 Architecture

ESP32 S3 ←→ WebSocket ←→ Firebase Hosting (Dashboard)

ESP32 S3 ←→ Firebase RTDB ←→ Cloud Run MCP ←→ Google ADK

Google ADK ←→ Gemini 2.5 Flash ←→ 7 AI Agents



\## ⚙️ Setup

1\. Clone repo

2\. Create `secrets.h` with your API keys

3\. Upload `SentiHome\_GEMINI\_API.ino` to ESP32 S3

4\. Deploy to Firebase: `firebase deploy`

5\. Run ADK agent: `adk run sentihome\_agent`



\## 📍 Made in Cameroon 🇨🇲

