<script setup>
import Hls from 'hls.js'

const dialog = inject('dialogRef')

const { id, model } = dialog.value.data

const store = useCameraStore()

let hls;
onMounted(async () => {
    await store.stream(id, true)
    var video = document.getElementById('video');

    if (Hls.isSupported()) {
        hls = new Hls();
        hls.loadSource(`/hls/${model}/stream.m3u8`);
        hls.attachMedia(video);
        hls.on(Hls.Events.MANIFEST_PARSED, function () {
            console.log('HLS Manifest parsed');
        });
    }

})
onBeforeUnmount(() => {
    hls.destroy()
    store.stream(id, false)
})
</script>

<template>
    <div class="flex flex-col gap-4">
        <video id="video" width="1080" height="640" autoplay />
        <div class="flex items-center justify-between">
            <Button :label="$t('back')" severity="secondary" outlined @click="dialog.close()" />
        </div>
    </div>
</template>