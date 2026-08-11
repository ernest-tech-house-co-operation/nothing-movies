import asyncio, json, uuid, websockets

async def main():
    async with websockets.connect("ws://127.0.0.1:2005") as ws:
        async def send(cmd, **payload):
            req_id = str(uuid.uuid4())
            await ws.send(json.dumps({"id": req_id, "cmd": cmd, "payload": payload}))
            while True:
                raw = await ws.recv()
                msg = json.loads(raw)
                if msg.get("id") == req_id:
                    return msg

        tab = await send("tab.new")
        tab_id = tab["data"]
        print("tabId:", tab_id)

        r1 = await send("evaluate", tabId=tab_id, js="Promise.resolve('hello')")
        print("Promise.resolve test:", r1)

        r2 = await send("evaluate", tabId=tab_id, js="'plain string'")
        print("plain string test:", r2)

        await send("tab.close", tabId=tab_id)

asyncio.run(main())