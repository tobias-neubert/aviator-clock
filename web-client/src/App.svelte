<script lang="ts">
  let wifiNetworks = [
    { ssid: 'Router-2.4GHz', signal: -45, protected: true },
    { ssid: 'GuestAccess', signal: -78, protected: false },
    { ssid: 'NeighbourWiFi', signal: -85, protected: true }
  ];
  
  let selectedSsid: string | null = null;
  let password = "";

  function toggleNetwork(ssid: string) {
    if (selectedSsid === ssid) {
      selectedSsid = null;
    } else {
      selectedSsid = ssid;
      password = "";
    }
  }

  function connect() {
    alert(`Connecting to ${selectedSsid}...`);
  }
</script>

<main>
  <div class="header">
    <h1>Aviator Clock</h1>
  </div>

  <div class="island">
    <h2>WLAN Networks</h2>
    <div class="list">
      {#each wifiNetworks as net}
        <!-- svelte-ignore a11y-click-events-have-key-events -->
        <!-- svelte-ignore a11y-no-static-element-interactions -->
        <div class="list-item" class:expanded={selectedSsid === net.ssid} on:click={() => toggleNetwork(net.ssid)}>
          <div class="row">
            <span class="ssid">{net.ssid}</span>
            <span class="signal">
                {#if net.protected}🔒{/if}
                {net.signal}dB
            </span>
          </div>
          
          {#if selectedSsid === net.ssid}
            <div class="details" on:click|stopPropagation>
               <input type="password" placeholder="Passwort" bind:value={password} />
               <button on:click={connect}>Verbinden</button>
            </div>
          {/if}
        </div>
      {/each}
    </div>
  </div>

  <div class="island">
    <h2>Status</h2>
    <div class="info-row">
        <span>IP Address</span>
        <span class="value">192.168.1.50</span>
    </div>
    <div class="info-row">
        <span>Uptime</span>
        <span class="value">00:15:32</span>
    </div>
    <div class="info-row">
        <span>Version</span>
        <span class="value">v1.2.0</span>
    </div>
  </div>
</main>

<style>
  main {
    display: flex;
    flex-direction: column;
    gap: 24px;
    padding-bottom: 40px;
  }

  .header h1 {
    font-size: 24px;
    font-weight: 700;
    margin: 10px 0;
    text-align: center;
    opacity: 0.9;
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

  .list {
    display: flex;
    flex-direction: column;
  }

  .list-item {
    border-bottom: 1px solid rgba(0,0,0,0.05);
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

  .details {
      padding-top: 16px;
      animation: slideDown 0.2s ease-out;
  }

  @keyframes slideDown {
      from { opacity: 0; transform: translateY(-5px); }
      to { opacity: 1; transform: translateY(0); }
  }

  .info-row {
      display: flex;
      justify-content: space-between;
      padding: 12px 0;
      border-bottom: 1px solid rgba(0,0,0,0.05);
      font-size: 15px;
  }
  
  .info-row:last-child {
      border-bottom: none;
  }
  
  .info-row .value {
      opacity: 0.6;
      font-family: monospace;
  }

  /* Dark mode specific overrides handled via opacity mostly, but borders need monitoring */
  @media (prefers-color-scheme: dark) {
      .list-item, .info-row {
          border-bottom-color: rgba(255,255,255,0.1);
      }
  }
</style>
