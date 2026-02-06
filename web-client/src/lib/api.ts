export interface WifiNetwork {
    ssid: string;
    rssi: number;
    protected: boolean;
}

export interface SystemStatus {
    ip: string;
    connected: boolean;
    mac?: string;
}

const IS_DEV = import.meta.env.DEV;

// Mock-Daten für Entwicklung
const MOCK_NETWORKS: WifiNetwork[] = [
    { ssid: "Home_WiFi", rssi: -50, protected: true },
    { ssid: "Guest_WiFi", rssi: -75, protected: false },
    { ssid: "Neighbor_WiFi", rssi: -90, protected: true },
];

const MOCK_STATUS: SystemStatus = {
    ip: "192.168.4.1",
    connected: false,
    mac: "AA:BB:CC:DD:EE:FF"
};

async function wait(ms: number) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

export const api = {
    async scanNetworks(): Promise<WifiNetwork[]> {
        if (IS_DEV) {
            console.log("Mock: Scanning networks...");
            await wait(1500);
            return MOCK_NETWORKS;
        }
        const res = await fetch('/api/wifi/scan');
        if (!res.ok) throw new Error("Scan failed");
        return await res.json();
    },

    async getStatus(): Promise<SystemStatus> {
        if (IS_DEV) {
            return MOCK_STATUS;
        }
        try {
            const res = await fetch('/api/system/status');
            if (res.ok) return await res.json();
        } catch (e) {
            // silent fail
        }
        return { ip: "0.0.0.0", connected: false };
    },

    async connect(ssid: string, pass: string): Promise<void> {
        if (IS_DEV) {
            console.log(`Mock: Connecting to ${ssid} with key ${pass}`);
            await wait(2000);
            return;
        }
        const res = await fetch('/api/wifi/connect', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ ssid, password: pass })
        });
        if (!res.ok) throw new Error("Connection failed");
    },
    
    async reboot(): Promise<void> {
         if (IS_DEV) {
            console.log("Mock: Rebooting...");
            return;
        }
        await fetch('/api/system/reboot', { method: 'POST' });
    },

    async finishSetup(): Promise<void> {
        if (IS_DEV) {
            console.log("Mock: Finishing setup...");
            return;
        }
        await fetch('/api/setup/finish', { method: 'POST' });
   }
};
