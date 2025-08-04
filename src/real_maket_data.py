import asyncio
import json
import websockets
import socket
import time

# --- Configuration ---
SYMBOL = "btcusd"  # Or another trading pair
WEBSOCKET_URI = "wss://ws-feed.pro.coinbase.com" # Example for Coinbase Pro
HOST = '127.0.0.1'
PORT = 8080

async def listen_for_market_data():
    """Connects to a WebSocket, receives data, and sends it to a C++ application."""
    async with websockets.connect(WEBSOCKET_URI) as websocket:
        print("Connected to WebSocket. Subscribing to market data...")

        # --- Subscription message for Coinbase Pro ---
        subscribe_message = {
            "type": "subscribe",
            "channels": [{"name": "ticker", "product_ids": [SYMBOL.upper()]}]
        }
        await websocket.send(json.dumps(subscribe_message))

        print("Subscription successful. Starting TCP server for C++ application...")
        # Start a simple TCP server to listen for a C++ client
        server = await asyncio.start_server(
            handle_client, HOST, PORT
        )

        async with server:
            # Continuously read from the WebSocket and send to the C++ client
            async for message in websocket:
                data = json.loads(message)
                if data['type'] == 'ticker':
                    try:
                        price = float(data['price'])
                        # Format the data for C++ (e.g., as a simple string)
                        formatted_data = f"PRICE:{price}\n"
                        await send_to_cpp(formatted_data)
                    except (KeyError, ValueError) as e:
                        print(f"Error parsing message: {e}")

async def handle_client(reader, writer):
    """Placeholder to handle client connections. For this simple case, we just send data."""
    global cpp_writer
    print("C++ client connected!")
    cpp_writer = writer
    # The main loop will now send data to this writer
    try:
        while True:
            # This loop could handle receiving data from C++, but for now, we just wait.
            await asyncio.sleep(1)
    except ConnectionError:
        print("C++ client disconnected.")
    finally:
        cpp_writer = None
        writer.close()

async def send_to_cpp(data):
    """Sends a message to the C++ client if connected."""
    global cpp_writer
    if cpp_writer:
        try:
            cpp_writer.write(data.encode())
            await cpp_writer.drain()
            print(f"Sent to C++: {data.strip()}")
        except (ConnectionError, BrokenPipeError) as e:
            print(f"Error sending to C++: {e}. Client likely disconnected.")
            cpp_writer = None

if __name__ == "__main__":
    cpp_writer = None
    try:
        asyncio.run(listen_for_market_data())
    except KeyboardInterrupt:
        print("Shutting down...")
    except Exception as e:
        print(f"An error occurred: {e}")