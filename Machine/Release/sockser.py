import asyncio
import websockets
import os.path

async def handler(websocket, path):
    data = await websocket.recv()
    reply = f"Data resivied as: {data}!"
    print(reply)
    print("Введите путь до файла или сообщение >>")
    #xt = input()
    xt = "m"
    try:
        file = open(xt)
    except IOError as e:
        await websocket.send(xt)
    else:
        xt = file.read()
        file.close()
        print("Send file")
        await websocket.send(xt)
        
async def main():
    async with websockets.serve(handler, "127.0.0.1", 27015):
        await asyncio.Future()

#start_server = websockets.serve(handler, "127.0.0.1", 27015)
#asyncio.get_event_loop().run_until_complete(start_server)
#asyncio.get_event_loop().run_forever()
asyncio.run(main())
