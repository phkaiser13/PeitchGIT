// src/bridge.ts

// This is a simulation of the `invoke` API from Tauri, adapted for binary payloads.
// The real implementation will be provided by the Tauri environment.
async function tauriInvoke(command: string, payload: Uint8Array): Promise<Uint8Array> {
  console.log(`[TAURI INVOKE]: command='${command}', payload=`, payload);
  // In a real environment, `window.__TAURI__.invoke` would be called here.
  // We will return an empty response for now as a placeholder.
  return new Uint8Array();
}

export async function invoke(command: string, payload: Uint8Array): Promise<Uint8Array> {
  try {
    const response = await tauriInvoke(command, payload);
    return response;
  } catch (error) {
    console.error(`[BRIDGE ERROR] while invoking '${command}':`, error);
    // Propagate the error so the service layer can handle it.
    throw new Error(`Communication with the backend failed: ${error instanceof Error ? error.message : String(error)}`);
  }
}
