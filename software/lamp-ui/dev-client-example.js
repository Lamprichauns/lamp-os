#!/usr/bin/env node

import WebSocket from 'ws'

const SERVER_URL = 'http://localhost:3001'
const WS_URL = 'ws://localhost:3001'

// Example HTTP POST request
async function sendPostRequest() {
  try {
    const response = await fetch(`${SERVER_URL}/debug/test`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        message: 'Hello from client!',
        timestamp: new Date().toISOString(),
        data: {
          userId: 123,
          action: 'test',
          metadata: { source: 'dev-client-example' },
        },
      }),
    })

    const result = await response.json()
    console.log('POST Response:', result)
  } catch (error) {
    console.error('POST Error:', error.message)
  }
}

// Example WebSocket connection
function connectWebSocket() {
  const ws = new WebSocket(WS_URL)

  ws.on('open', () => {
    console.log('WebSocket connected')

    // Send a test message
    ws.send(
      JSON.stringify({
        type: 'test',
        message: 'Hello WebSocket!',
        timestamp: new Date().toISOString(),
      }),
    )
  })

  ws.on('message', (data) => {
    const message = JSON.parse(data)
    console.log('WebSocket received:', message)
  })

  ws.on('close', () => {
    console.log('WebSocket disconnected')
  })

  ws.on('error', (error) => {
    console.error('WebSocket error:', error)
  })

  return ws
}

// Main function
async function main() {
  console.log('=== Development Client Example ===\n')

  // Test HTTP POST
  console.log('1. Testing HTTP POST...')
  await sendPostRequest()

  console.log('\n2. Testing WebSocket...')
  const ws = connectWebSocket()

  // Keep the process alive for WebSocket testing
  setTimeout(() => {
    console.log('\nClosing WebSocket connection...')
    ws.close()
    process.exit(0)
  }, 5000)
}

// Run if this file is executed directly
if (import.meta.url === `file://${process.argv[1]}`) {
  main().catch(console.error)
}

export { sendPostRequest, connectWebSocket }
