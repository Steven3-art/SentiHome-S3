import json
import urllib.request
from google.adk import Agent

FIREBASE_URL = "https://gen-lang-client-0279809768-default-rtdb.firebaseio.com"


def get_home_status() -> dict:
    """Get real-time sensor data and relay states from SentiHome S3."""
    try:
        url = f"{FIREBASE_URL}/sentihome/sensors.json"
        with urllib.request.urlopen(url, timeout=5) as r:
            sensors = json.loads(r.read())
        url2 = f"{FIREBASE_URL}/sentihome/relays.json"
        with urllib.request.urlopen(url2, timeout=5) as r:
            relays = json.loads(r.read())
        return {"sensors": sensors, "relays": relays, "status": "online"}
    except Exception as e:
        return {"error": str(e)}

def control_relay(zone: str, state: bool) -> dict:
    """Control a light zone. zone='1','2','3'. state=True ON, False OFF."""
    try:
        url = f"{FIREBASE_URL}/sentihome/commands/relay{zone}.json"
        data = json.dumps(state).encode()
        req = urllib.request.Request(
            url, data=data, method='PUT',
            headers={'Content-Type': 'application/json'})
        with urllib.request.urlopen(req, timeout=5):
            pass
        return {
            "success": True,
            "zone": zone,
            "state": state,
            "message": f"Zone {zone} {'allumée' if state else 'éteinte'} !"
        }
    except Exception as e:
        return {"error": str(e)}

def control_all_lights(state: bool) -> dict:
    """Control all 3 light zones. state=True ALL ON, False ALL OFF."""
    try:
        for i in range(1, 4):
            url = f"{FIREBASE_URL}/sentihome/commands/relay{i}.json"
            data = json.dumps(state).encode()
            req = urllib.request.Request(
                url, data=data, method='PUT',
                headers={'Content-Type': 'application/json'})
            with urllib.request.urlopen(req, timeout=5):
                pass
        return {
            "success": True,
            "state": state,
            "message": f"Toutes zones {'allumées' if state else 'éteintes'} !"
        }
    except Exception as e:
        return {"error": str(e)}

root_agent = Agent(
    name="sentihome_coordinator",
    model="gemini-2.5-flash-lite",
    instruction="""
Tu es SentiHome S3 Coordinator, intelligence maître
d'une maison intelligente à Yaoundé, Cameroun.
Tu coordonnes 7 agents IA : AURA, LUX, RADAR,
PURETÉ, VOLT, CONNECT, VISION.
Utilise TOUJOURS get_home_status pour lire les données.
Utilise control_relay pour contrôler une zone.
Utilise control_all_lights pour tout contrôler.
Réponds en français avec les vraies valeurs !
""",
    tools=[get_home_status, control_relay, control_all_lights]
)