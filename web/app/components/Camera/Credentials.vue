<script setup>
const dialog = inject('dialogRef')

const { id } = dialog.value.data

const toast = useToast()

const form = reactive({ username: '', password: '' })

const readonly = ref(true)

const { pending, execute } = useFetch(`/api/cameras/${id}/credentials`, {
    immediate: false, method: 'POST', body: form,
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
            <InputText v-model="form.username" fluid class="ltr" :readonly />
            <label>{{ $t('username') }}</label>
        </FloatLabel>
        <FloatLabel variant="on">
            <Password v-model="form.password" :feedback="false" fluid class="ltr" toggle-mask :readonly
                :class="{ '[&>svg]:hidden!': !form.password }" />
            <label>{{ $t('password') }}</label>
        </FloatLabel>
        <div class="w-90 flex items-center justify-between">
            <Button :label="$t('back')" severity="secondary" outlined @click="dialog.close()" />
            <Button :label="$t('submit')" type="submit" :loading="pending" />
        </div>
    </form>
</template>
