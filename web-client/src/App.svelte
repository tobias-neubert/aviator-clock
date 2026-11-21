<script lang="ts">
  import { onMount } from 'svelte';
  import {
    api,
    type WifiNetwork,
    type SystemStatus,
    type AlarmConfiguration,
    type AudioTrack
  } from './lib/api';

  let wifiNetworks: WifiNetwork[] = [];
  let status: SystemStatus = { ip: '...', connected: false };
  let audioTracks: AudioTrack[] = [];

  let selectedSsid: string | null = null;
  let password = "";

  let isScanning = false;
  let isConnecting = false;
  let connectionMessage = "";
  let isConnectionError = false;
  let setupSuccess = false;
  let theme = 'light';

  // Alarm settings state
  let alarmTime = "07:30";
  let alarmEnabled = true;
  let selectedTrackId = 1;
  let isSavingAlarm = false;
  let isPreviewPlaying = false;
  let alarmStatusMessage = "";
  let isAlarmError = false;

  onMount(async () => {
    // Theme initialization
    const savedTheme = localStorage.getItem('theme');
    if (savedTheme) {
      theme = savedTheme;
    } else if (window.matchMedia('(prefers-color-scheme: dark)').matches) {
      theme = 'dark';
    }
    applyTheme();

    await Promise.all([
      loadStatus(),
      loadAlarm(),
      loadAudioTracks(),
      scanWifi()
    ]);
  });

  function applyTheme() {
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem('theme', theme);
  }

  function toggleTheme() {
    theme = theme === 'dark' ? 'light' : 'dark';
    applyTheme();
  }

  async function loadStatus() {
    try {
      status = await api.getStatus();
    } catch (error) {
      console.error("Failed to load system status", error);
    }
  }

  async function loadAlarm() {
    try {
      const alarm: AlarmConfiguration = await api.getAlarm();
      const paddedHour = String(alarm.hour).padStart(2, '0');
      const paddedMinute = String(alarm.minute).padStart(2, '0');
      alarmTime = `${paddedHour}:${paddedMinute}`;
      alarmEnabled = alarm.enabled;
      selectedTrackId = alarm.soundTrackId || 1;
    } catch (error) {
      console.error("Failed to load alarm configuration", error);
    }
  }

  async function loadAudioTracks() {
    try {
      audioTracks = await api.getAudioTracks();
      if (audioTracks.length > 0 && !selectedTrackId) {
        selectedTrackId = audioTracks[0].id;
      }
    } catch (error) {
      console.error("Failed to load audio tracks", error);
    }
  }

  async function scanWifi() {
    isScanning = true;
    try {
      wifiNetworks = await api.scanNetworks();
    } catch (error) {
      console.error("Wi-Fi scan failed", error);
    } finally {
      isScanning = false;
    }
  }

  function toggleNetwork(ssid: string) {
    if (selectedSsid === ssid) {
      selectedSsid = null;
    } else {
      selectedSsid = ssid;
      password = "";
      connectionMessage = "";
      isConnectionError = false;
    }
  }

  async function connect() {
    if (!selectedSsid) return;

    isConnecting = true;
    connectionMessage = "";
    isConnectionError = false;

    try {
      await api.connect(selectedSsid, password);
      setupSuccess = true;

      setTimeout(async () => {
        await api.finishSetup();
      }, 1000);
    } catch (error) {
      isConnectionError = true;
      connectionMessage = "Failed to save Wi-Fi credentials.";
    } finally {
      isConnecting = false;
    }
  }

  async function saveAlarm() {
    isSavingAlarm = true;
    alarmStatusMessage = "";
    isAlarmError = false;

    try {
      const [hourString, minuteString] = alarmTime.split(':');
      const hour = parseInt(hourString, 10) || 0;
      const minute = parseInt(minuteString, 10) || 0;

      await api.updateAlarm({
        hour,
        minute,
        soundTrackId: Number(selectedTrackId),
        enabled: alarmEnabled
      });

      alarmStatusMessage = "Alarm configuration saved.";
    } catch (error) {
      isAlarmError = true;
      alarmStatusMessage = "Failed to save alarm settings.";
    } finally {
      isSavingAlarm = false;
    }
  }

  async function toggleAudioPreview() {
    try {
      if (isPreviewPlaying) {
        await api.stopAudio();
        isPreviewPlaying = false;
      } else {
        await api.testAudio(Number(selectedTrackId));
        isPreviewPlaying = true;
      }
    } catch (error) {
      console.error("Failed to toggle audio preview", error);
    }
  }
</script>

{#if setupSuccess}
  <div class="success-overlay">
    <div class="island success-island">
      <div class="success-content">
        <h2 class="done-title">Done.</h2>
        <p class="success-desc">Settings saved.<br>The clock will now connect.</p>
      </div>
      <div class="success-footer">
        <p class="small">You may close this window now.</p>
      </div>
    </div>
  </div>
{:else}
<main>
  <div class="header">
    <h1>Aviator Clock</h1>
    <button class="theme-btn" on:click={toggleTheme} aria-label="Toggle Theme">
      {theme === 'dark' ? '🌙' : '☀️'}
    </button>
  </div>

  <!-- Alarm Configuration Card -->
  <div class="island">
    <div class="row">
      <h2>Alarm Configuration</h2>
      <label class="switch" title="Toggle Alarm On/Off">
        <input type="checkbox" bind:checked={alarmEnabled} />
        <span class="slider round"></span>
      </label>
    </div>

    <div class="form-group">
      <label class="label-text" for="alarm-time">Alarm Time</label>
      <input
        id="alarm-time"
        type="time"
        bind:value={alarmTime}
        class="input-field"
      />
    </div>

    <div class="form-group">
      <label class="label-text" for="sound-track">Sound Track</label>
      <select id="sound-track" bind:value={selectedTrackId} class="select-field">
        {#each audioTracks as track}
          <option value={track.id}>{track.name}</option>
        {/each}
      </select>
    </div>

    <div class="button-row">
      <button
        class="secondary-btn"
        on:click={toggleAudioPreview}
        type="button"
      >
        {isPreviewPlaying ? '⏹ Stop Preview' : '▶ Test Play'}
      </button>
      <button on:click={saveAlarm} disabled={isSavingAlarm} type="button">
        {isSavingAlarm ? 'Saving...' : 'Save Alarm'}
      </button>
    </div>

    {#if alarmStatusMessage}
      <p class="msg" class:msg-error={isAlarmError}>{alarmStatusMessage}</p>
    {/if}
  </div>

  <!-- Wi-Fi Networks Card -->
  <div class="island">
    <div class="row">
      <h2>Wi-Fi Networks</h2>
      <button class="icon-btn" on:click={scanWifi} disabled={isScanning} title="Rescan Networks">
        {isScanning ? '...' : '↻'}
      </button>
    </div>

    <div class="list">
      {#if isScanning && wifiNetworks.length === 0}
        <div class="list-item center">Scanning networks...</div>
      {:else if wifiNetworks.length === 0}
        <div class="list-item center">No networks found</div>
      {/if}

      {#each wifiNetworks as net}
        <!-- svelte-ignore a11y-click-events-have-key-events -->
        <!-- svelte-ignore a11y-no-static-element-interactions -->
        <div
          class="list-item"
          class:expanded={selectedSsid === net.ssid}
          on:click={() => toggleNetwork(net.ssid)}
        >
          <div class="row">
            <span class="ssid">{net.ssid}</span>
            <span class="signal">
              {#if net.protected}🔒{/if}
              {net.rssi}dB
            </span>
          </div>

          {#if selectedSsid === net.ssid}
            <div class="details" on:click|stopPropagation>
              <input
                type="password"
                placeholder="Wi-Fi Password"
                class:input-error={isConnectionError}
                bind:value={password}
                disabled={isConnecting}
              />
              <button on:click={connect} disabled={isConnecting}>
                {isConnecting ? 'Connecting...' : 'Connect'}
              </button>
              {#if connectionMessage}
                <p class="msg" class:msg-error={isConnectionError}>{connectionMessage}</p>
              {/if}
            </div>
          {/if}
        </div>
      {/each}
    </div>
  </div>

  <!-- System Status Card -->
  <div class="island">
    <h2>System Status</h2>
    <div class="info-row">
      <span>IP Address</span>
      <span class="value">{status.ip}</span>
    </div>
    <div class="info-row">
      <span>Connected</span>
      <span class="value">{status.connected ? 'Yes' : 'No'}</span>
    </div>
    {#if status.operatingMode}
      <div class="info-row">
        <span>Operating Mode</span>
        <span class="value">{status.operatingMode}</span>
      </div>
    {/if}
    {#if status.isAlarmRinging !== undefined}
      <div class="info-row">
        <span>Alarm Ringing</span>
        <span class="value">{status.isAlarmRinging ? 'Active Ringing' : 'Idle'}</span>
      </div>
    {/if}
  </div>
</main>
{/if}

<style>
  /* --- Success Overlay Styles --- */
  .success-overlay {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: var(--bg-color);
    display: flex;
    justify-content: center;
    align-items: center;
    z-index: 1000;
    padding: 15px;
    box-sizing: border-box;
  }

  .success-island {
    width: 100%;
    height: 100%;
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    align-items: center;
    text-align: center;
    box-sizing: border-box;
    padding: 40px 20px 20px 20px;
  }

  .success-content {
    flex: 1;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
  }

  .done-title {
    font-size: 48px;
    font-weight: 800;
    color: var(--primary-color);
    margin: 0 0 16px 0;
    opacity: 1 !important;
    text-transform: none !important;
  }

  .success-desc {
    font-size: 18px;
    opacity: 0.7;
    line-height: 1.5;
  }

  .success-footer {
    width: 100%;
    padding-top: 20px;
  }

  .small {
    font-size: 14px;
    opacity: 0.5;
  }

  /* --- Main Layout --- */
  main {
    display: flex;
    flex-direction: column;
    gap: 24px;
    padding-bottom: 40px;
  }

  .header {
    position: relative;
    display: flex;
    justify-content: center;
    align-items: center;
    margin-bottom: 10px;
  }

  .header h1 {
    font-size: 24px;
    font-weight: 700;
    margin: 10px 0;
    text-align: center;
    opacity: 0.9;
  }

  .theme-btn {
    position: absolute;
    right: 0;
    background: none;
    border: none;
    font-size: 22px;
    padding: 8px;
    cursor: pointer;
    opacity: 0.8;
    transition: transform 0.2s;
  }

  .theme-btn:active {
    transform: scale(0.9);
  }

  .island {
    background-color: var(--surface-color);
    border-radius: var(--border-radius);
    padding: 20px;
    box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.05), 0 2px 4px -1px rgba(0, 0, 0, 0.03);
  }

  .island h2 {
    font-size: 15px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    font-weight: 600;
    margin: 0 0 12px 0;
    opacity: 0.5;
  }

  .form-group {
    display: flex;
    flex-direction: column;
    margin-bottom: 12px;
  }

  .label-text {
    font-size: 14px;
    font-weight: 500;
    opacity: 0.7;
    margin-bottom: 4px;
  }

  .input-field, .select-field {
    width: 100%;
    padding: 10px 12px;
    background-color: var(--bg-color);
    color: var(--text-color);
    border: 1px solid transparent;
    border-radius: 8px;
    box-sizing: border-box;
    font-size: 16px;
    outline: none;
    transition: border-color 0.2s;
  }

  .input-field:focus, .select-field:focus {
    border-color: var(--primary-color);
  }

  .button-row {
    display: flex;
    gap: 12px;
    margin-top: 14px;
  }

  .button-row button {
    flex: 1;
  }

  .secondary-btn {
    background-color: transparent;
    color: var(--primary-color);
    border: 1px solid var(--primary-color);
  }

  .secondary-btn:active {
    background-color: rgba(0, 122, 255, 0.1);
  }

  /* --- Toggle Switch --- */
  .switch {
    position: relative;
    display: inline-block;
    width: 48px;
    height: 28px;
  }

  .switch input {
    opacity: 0;
    width: 0;
    height: 0;
  }

  .slider {
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: #ccc;
    transition: 0.3s;
    border-radius: 28px;
  }

  .slider:before {
    position: absolute;
    content: "";
    height: 22px;
    width: 22px;
    left: 3px;
    bottom: 3px;
    background-color: white;
    transition: 0.3s;
    border-radius: 50%;
  }

  input:checked + .slider {
    background-color: var(--primary-color);
  }

  input:checked + .slider:before {
    transform: translateX(20px);
  }

  /* --- List and Rows --- */
  .list {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .center {
    display: flex;
    justify-content: center;
    align-items: center;
    color: var(--text-color);
    opacity: 0.5;
    font-style: italic;
  }

  .icon-btn {
    background: none;
    color: var(--text-color);
    padding: 0 8px;
    font-size: 20px;
    opacity: 0.5;
    width: auto;
  }

  .msg {
    text-align: center;
    font-size: 14px;
    margin-top: 12px;
    color: var(--primary-color);
  }

  .msg-error {
    color: #ff3e3e;
  }

  .input-error {
    border-color: #ff3e3e !important;
    background-color: rgba(255, 62, 62, 0.05);
  }

  .details {
    display: flex;
    flex-direction: column;
    margin-top: 10px;
    gap: 10px;
    padding-top: 16px;
    animation: slideDown 0.2s ease-out;
  }

  .list-item {
    border-bottom: 1px solid rgba(0, 0, 0, 0.05);
    padding: 14px 0;
    cursor: pointer;
    transition: background-color 0.2s;
  }

  .list-item:last-child {
    border-bottom: none;
  }

  .row {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .ssid {
    font-weight: 500;
    font-size: 17px;
  }

  .signal {
    font-size: 13px;
    opacity: 0.5;
  }

  @keyframes slideDown {
    from { opacity: 0; transform: translateY(-5px); }
    to { opacity: 1; transform: translateY(0); }
  }

  .info-row {
    display: flex;
    justify-content: space-between;
    padding: 12px 0;
    border-bottom: 1px solid rgba(0, 0, 0, 0.05);
    font-size: 15px;
  }

  .info-row:last-child {
    border-bottom: none;
  }

  .info-row .value {
    opacity: 0.6;
    font-family: monospace;
  }

  @media (prefers-color-scheme: dark) {
    .list-item, .info-row {
      border-bottom-color: rgba(255, 255, 255, 0.1);
    }
  }
</style>
