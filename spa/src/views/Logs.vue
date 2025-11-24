<script setup>
const scroll = ref()

const log = useLogStore()

let timer;
let flag = false;
let count = 0;
const getLogs = async (line) => {
    await log.index(line)
    timer = setTimeout(() => {
        getLogs(log.items.length + 1)
    }, 1000);
    if (count < log.items.length)
        nextTick(() => {
            scroll.value.scrollTo({
                top: scroll.value.scrollTop + scroll.value.scrollHeight,
                behavior: flag ? 'smooth' : 'instant'
            })
            flag = true
        })
    count = log.items.length
}

onMounted(() => getLogs())

onUnmounted(() => clearTimeout(timer));
</script>

<template>
    <div class="bg-white rounded p-6 flex flex-col gap-8 overflow-hidden">
        <LogHeader />
        <div ref="scroll" class="shadow-inner rounded-lg bg-gray-950 overflow-y-auto h-[50vh] p-6 ltr">
            <ul class="text-white flex flex-col gap-1 font-['arial']">
                <li v-for="item in log.items" class="flex gap-2">
                    <span class="text-amber-400 whitespace-nowrap">[{{ item.date }}]</span>
                    <pre>{{ item.message }}</pre>
                </li>
            </ul>
        </div>
    </div>
</template>