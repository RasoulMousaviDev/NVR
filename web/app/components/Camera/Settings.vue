<script setup>
const dialog = inject('dialogRef')

const data = dialog.value.data

const toast = useToast()

const form = reactive({
    image_quality: data.image_quality,
    audio_quality: data.audio_quality || 'off',
    duration: data.duration,
})

const iamge_qualities = reactive(['low', 'medium', 'high', 'ultra'])

const audio_qualities = reactive(['off', 'low', 'medium', 'high', 'ultra'])

const readonly = ref(true)

const { pending, execute } = useFetch(`/api/cameras/${data.id}`, {
    immediate: false, method: 'PATCH', body: form,
    onResponse({ response }) {
        if (response.ok) {
            toast.add({ severity: 'success', summary: 'Success', detail: response._data.message, life: 3000 })
            Object.assign(data, form)
            dialog.value.close()
        } else {
            toast.add({ severity: 'error', summary: 'Error', detail: response._data.message, life: 3000 })
        }
    }
})
</script>

<template>
    <form class="flex flex-col gap-8 pt-4" @submit.prevent="execute()" @click="readonly = false">
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
                <span>{{ $t('minute') }}</span>
            </InputGroupAddon>
        </InputGroup>
        <div class="w-90 flex items-center justify-between">
            <Button :label="$t('back')" severity="secondary" outlined @click="dialog.close()" />
            <Button :label="$t('save')" type="submit" :loading="pending" />
        </div>
    </form>
</template>
