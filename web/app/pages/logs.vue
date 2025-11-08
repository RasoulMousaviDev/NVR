<script setup>
let ws;
const scroll = ref()
const lines = ref([])
onMounted(() => {
    ws = new WebSocket(`ws://${location.host}/api/logs/ws`);
    let flag = false
    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        if (data.lines) lines.value.push(...data.lines);

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
        <div ref="scroll" class="shadow-inner rounded-lg bg-primary overflow-y-auto h-[50vh] p-6 ltr">
            <ul class="text-white flex flex-col gap-1 font-['arial']">
                <li v-for="line in lines">
                    {{ line }}
                </li>
            </ul>
        </div>
    </div>
</template>