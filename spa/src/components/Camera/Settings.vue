<script setup>
const { toast } = inject('services')

const dialog = inject('dialogRef')

const store = useCameraStore()

const data = dialog.value.data

const form = reactive({
    image_quality: data.image_quality,
    duration: data.duration,
    audio_quality: data.audio_quality || 'off',
})

const iamge_qualities = reactive(['low', 'medium', 'high', 'ultra'])

const audio_qualities = reactive(['off', 'low', 'medium', 'high', 'ultra'])

const readonly = ref(true)

const handleClick = async () => {

    const status = await store.settings(data.id, form)

    if (status === 200) {
        toast.add({ severity: 'success', summary: 'Success', detail: 'Updated', life: 3000 })
        Object.assign(data, form)
        dialog.value.close()
    } else {
        toast.add({ severity: 'error', summary: 'Error', detail: 'Failed', life: 3000 })
    }
}
</script>

<template>
    <form class="flex flex-col gap-8 pt-4" @submit.prevent="handleClick()" @click="readonly = false">
        <FloatLabel variant="on">
            <Select v-model="form.image_quality" :options="iamge_qualities" :option-label="$t" fluid :readonly />
            <label>{{ $t('iamge-quality') }}</label>
        </FloatLabel>
        <FloatLabel v-if="data.audio" variant="on">
            <Select v-model="form.audio_quality" :options="audio_qualities" :option-label="$t" fluid :readonly />
            <label>{{ $t('audio-quality') }}</label>
        </FloatLabel>
        <InputGroup>
            <FloatLabel variant="on">
                <InputNumber v-model="form.duration" fluid input-class="text-left" show-buttons min="1" max="60" />
                <label>{{ $t('duration') }}</label>
            </FloatLabel>
            <InputGroupAddon>
                <span>{{ $t('second') }}</span>
            </InputGroupAddon>
        </InputGroup>
        <div class="w-90 flex items-center justify-between">
            <Button :label="$t('back')" severity="secondary" outlined @click="dialog.close()" />
            <Button :label="$t('save')" type="submit" :loading="store.loading" />
        </div>
    </form>
</template>
