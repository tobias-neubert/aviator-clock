export interface WifiNetwork {
    ssid: string;
    rssi: number;
    protected: boolean;
}

export interface SystemStatus {
    ip: string;
    connected: boolean;
    mac?: string;
    operatingMode?: string;
    isWifiConnected?: boolean;
    isTimeSynchronized?: boolean;
    isAlarmRinging?: boolean;
}

export interface AlarmConfiguration {
    hour: number;
    minute: number;
    soundTrackId: number;
    enabled: boolean;
}

export interface AudioTrack {
    id: number;
    name: string;
}

const IS_DEV = import.meta.env.DEV;

const MOCK_NETWORKS: WifiNetwork[] = [
    { ssid: "Home_WiFi", rssi: -50, protected: true },
    { ssid: "Guest_WiFi", rssi: -75, protected: false },
    { ssid: "Aviation_Club", rssi: -82, protected: true },
];

const MOCK_STATUS: SystemStatus = {
    ip: "192.168.4.1",
    connected: false,
    operatingMode: "SetupMode",
    isWifiConnected: false,
    isTimeSynchronized: false,
    isAlarmRinging: false
};

const MOCK_AUDIO_TRACKS: AudioTrack[] = [
    { id: 1, name: "Alarm Tone 1 (Chime)" },
    { id: 2, name: "Alarm Tone 2 (Classic Bell)" },
    { id: 3, name: "Alarm Tone 3 (Aviation Alert)" },
    { id: 4, name: "Alarm Tone 4 (Gentle Melody)" },
    { id: 5, name: "Alarm Tone 5 (Digital Pulse)" },
];

let mockAlarm: AlarmConfiguration = {
    hour: 7,
    minute: 30,
    soundTrackId: 1,
    enabled: true
};

async function wait(milliseconds: number): Promise<void> {
    return new Promise(resolve => setTimeout(resolve, milliseconds));
}

export const api = {
    async scanNetworks(): Promise<WifiNetwork[]> {
        if (IS_DEV) {
            console.log("Mock: Scanning networks...");
            await wait(1200);
            return MOCK_NETWORKS;
        }
        const response = await fetch('/api/wifi/scan');
        if (!response.ok) {
            throw new Error("Failed to scan Wi-Fi networks");
        }
        return await response.json();
    },

    async getStatus(): Promise<SystemStatus> {
        if (IS_DEV) {
            return MOCK_STATUS;
        }
        try {
            const response = await fetch('/api/system/status');
            if (response.ok) {
                return await response.json();
            }
        } catch (error) {
            // Silently fall back to default offline status
        }
        return { ip: "0.0.0.0", connected: false };
    },

    async connect(ssid: string, pass: string): Promise<void> {
        if (IS_DEV) {
            console.log(`Mock: Connecting to ${ssid}`);
            await wait(1500);
            return;
        }
        const response = await fetch('/api/wifi/connect', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ ssid, password: pass })
        });
        if (!response.ok) {
            throw new Error("Failed to connect Wi-Fi");
        }
    },

    async reboot(): Promise<void> {
        if (IS_DEV) {
            console.log("Mock: Rebooting device...");
            return;
        }
        await fetch('/api/system/reboot', { method: 'POST' });
    },

    async finishSetup(): Promise<void> {
        if (IS_DEV) {
            console.log("Mock: Finishing setup mode...");
            return;
        }
        await fetch('/api/setup/finish', { method: 'POST' });
    },

    async getAlarm(): Promise<AlarmConfiguration> {
        if (IS_DEV) {
            return { ...mockAlarm };
        }
        const response = await fetch('/api/alarm');
        if (!response.ok) {
            throw new Error("Failed to fetch alarm configuration");
        }
        return await response.json();
    },

    async updateAlarm(alarm: AlarmConfiguration): Promise<void> {
        if (IS_DEV) {
            mockAlarm = { ...alarm };
            console.log("Mock: Updated alarm configuration", mockAlarm);
            await wait(400);
            return;
        }
        const response = await fetch('/api/alarm', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(alarm)
        });
        if (!response.ok) {
            throw new Error("Failed to update alarm configuration");
        }
    },

    async getAudioTracks(): Promise<AudioTrack[]> {
        if (IS_DEV) {
            return MOCK_AUDIO_TRACKS;
        }
        const response = await fetch('/api/audio/tracks');
        if (!response.ok) {
            throw new Error("Failed to fetch audio tracks");
        }
        return await response.json();
    },

    async testAudio(trackId: number): Promise<void> {
        if (IS_DEV) {
            console.log(`Mock: Playing preview for track ${trackId}`);
            return;
        }
        const response = await fetch('/api/audio/test', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ trackId })
        });
        if (!response.ok) {
            throw new Error("Failed to test audio");
        }
    },

    async stopAudio(): Promise<void> {
        if (IS_DEV) {
            console.log("Mock: Stopped audio preview");
            return;
        }
        const response = await fetch('/api/audio/test', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ trackId: 0, action: 'stop' })
        });
        if (!response.ok) {
            throw new Error("Failed to stop audio preview");
        }
    }
};
