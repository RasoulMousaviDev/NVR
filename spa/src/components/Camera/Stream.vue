<script setup>
const dialog = inject('dialogRef')

// const { url } = dialog.value.data

const img = ref()

const url = ref('http://192.168.1.100:3000/stream.jpg')

const date = ref()

let timer;

onMounted(() => timer= setInterval(() => {
    date.value = Date.now()
}, 300))

onBeforeUnmount(() => clearInterval(timer))
</script>

<template>
    <div class="flex flex-col gap-4">
        {{ url }}
        <img ref="img" width="1080" height="640" :src="`${url}?${date}`" />
        <div class="flex items-center justify-between">
            <Button :label="$t('back')" severity="secondary" outlined @click="dialog.close()" />
        </div>
    </div>
</template>