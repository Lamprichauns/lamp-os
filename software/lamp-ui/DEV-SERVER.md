# Development Server

A simple Node.js development server for debugging and logging HTTP POST requests and WebSocket connections.

## Features

- **HTTP POST Support**: Accepts POST requests to any endpoint with JSON payloads
- **WebSocket Support**: Real-time bidirectional communication
- **Comprehensive Logging**: All requests and messages are logged to console and broadcast via WebSocket
- **CORS Enabled**: Allows cross-origin requests for development
- **Health Check**: Simple health endpoint for monitoring

## Quick Start

1. **Install dependencies** (if not already done):

   ```bash
   npm ci
   ```

2. **Start the development server**:

   ```bash
   npm run dev:server
   ```

3. **Server will be available at**:
   - HTTP: `http://localhost:3001`
   - WebSocket: `ws://localhost:3001`
   - Health Check: `http://localhost:3001/health`

## Usage Examples

### HTTP POST Requests

Send POST requests to any endpoint:

```javascript
// From your frontend or any HTTP client
fetch('http://localhost:3001/debug/my-endpoint', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
  },
  body: JSON.stringify({
    message: 'Hello from client!',
    data: { userId: 123, action: 'test' },
  }),
})
  .then((response) => response.json())
  .then((data) => console.log('Response:', data))
```

### WebSocket Connection

```javascript
const ws = new WebSocket('ws://localhost:3001')

ws.onopen = () => {
  console.log('Connected to dev server')

  // Send a message
  ws.send(
    JSON.stringify({
      type: 'debug',
      message: 'Hello WebSocket!',
      data: { timestamp: Date.now() },
    }),
  )
}

ws.onmessage = (event) => {
  const message = JSON.parse(event.data)
  console.log('Received:', message)
}
```

### Test with Example Client

Run the included example client:

```bash
node dev-client-example.js
```

## API Endpoints

### Health Check

- **GET** `/health`
- Returns server status and connection count

### Any Other Endpoint

- **GET** `/*` - Logs and returns request info
- **POST** `/*` - Logs request body and returns confirmation

## WebSocket Message Format

All WebSocket messages are JSON with the following structure:

```json
{
  "timestamp": "2024-01-01T12:00:00.000Z",
  "type": "message_type",
  "message": "Human readable message",
  "data": {
    /* optional data object */
  }
}
```

## Logging

All activity is logged to the console with timestamps and broadcast to connected WebSocket clients in real-time. Log types include:

- `REQUEST` - HTTP requests
- `POST` - POST request details
- `GET` - GET request details
- `WEBSOCKET` - WebSocket connections/disconnections
- `WEBSOCKET_MESSAGE` - WebSocket messages
- `WEBSOCKET_ERROR` - WebSocket errors
- `SERVER` - Server events
- `ERROR` - Server errors

## Configuration

Set environment variables to customize the server:

```bash
# Change port (default: 3001)
DEV_SERVER_PORT=3002 npm run dev:server
```

## Stopping the Server

Press `Ctrl+C` to gracefully stop the server.

## Use Cases

- **API Testing**: Test POST endpoints during development
- **WebSocket Debugging**: Monitor real-time communication
- **Request Logging**: See all incoming requests with full details
- **Development Proxy**: Forward requests to your main application
- **Client Testing**: Test client-side code against a simple server

## Security Note

⚠️ **This server is for development only!** It accepts all requests and has no authentication or security measures. Never use in production.
