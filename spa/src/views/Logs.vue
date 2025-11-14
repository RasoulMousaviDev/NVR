<script setup>
let ws;
const scroll = ref()
const lines = ref([])
onMounted(() => {
    ws = new WebSocket(`ws://${location.host}/api/logs/ws`);
    let flag = false
    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        console.log(data);
        
        if (data.lines && data.lines.length > 0) lines.value.push(...data.lines);

        nextTick(() => {
            scroll.value.scrollTo({
                top: scroll.value.scrollTop + scroll.value.scrollHeight,
                behavior: flag ? 'smooth' : 'instant'
            })
            flag = true
        })
    };
    scroll.value.scrollTo({ top: 1000 })
});

onUnmounted(() => {
    ws?.close(1000, 'Component unmounted');
});
</script>

<template>
    <div class="bg-white rounded p-6 flex flex-col gap-8 overflow-hidden">
        <LogHeader />
        <div ref="scroll" class="shadow-inner rounded-lg bg-gray-950 overflow-y-auto h-[50vh] p-6 ltr">
            <ul class="text-white flex flex-col gap-1 font-['arial']">
                <li v-for="line in lines" class="flex gap-2">
                    <span class="text-amber-400 whitespace-nowrap">[{{ line.date }}]</span>
                    <pre>{{ line.text }}</pre>
                </li>
            </ul>
        </div>
    </div>
</template>