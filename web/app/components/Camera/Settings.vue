<script setup>
const dialog = inject('dialogRef')

const data = dialog.value.data

const toast = useToast()

const form = reactive({
    quality: data.quality,
    duration: data.duration,
    audio: data.audio,
})

const qualities = reactive(['low', 'medium', 'high', 'ultra'])

const readonly = ref(true)

const { pending, execute } = useFetch(`/api/cameras/${data.id}`, {
    immediate: false, method: 'PATCH', body: form,
    onResponse({ response }) {
        if (response.ok) {
            toast.add({ severity: 'success', summary: 'Success', detail: '', life: 3000 })
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
            <Select v-model="form.quality" :options="qualities" :option-label="$t" fluid :readonly />
            <label>{{ $t('quality') }}</label>
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

        <label class="flex items-center justify-between cursor-pointer pl-2">
            <span>{{ $t('audio') }}</span>
            <InputSwitch v-model="form.audio" />
        </label>
        <div class="w-90 flex items-center justify-between">
            <Button :label="$t('back')" severity="secondary" outlined @click="dialog.close()" />
            <Button :label="$t('save')" type="submit" :loading="pending" />
        </div>
    </form>
</template>
