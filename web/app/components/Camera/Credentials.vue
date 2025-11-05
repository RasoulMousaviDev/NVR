<script setup>
const dialog = inject('dialogRef')

const { id } = dialog.value.data

const toast = useToast()

const form = reactive({ username: '', password: '' })

const { pending, execute } = useFetch(`/api/cameras/${id}/credentials`, {
    immediate: false, method: 'POST', body: form, 
    onResponse(){

        dialog.value.close()
        toast.add({ severity: 'success', summary: 'Success', detail: '', life: 3000})
    }
})
</script>

<template>
    <form class="flex flex-col gap-8 pt-4" @submit.prevent="execute()">
        <FloatLabel variant="on">
            <InputText v-model="form.username" fluid class="ltr" />
            <label>{{ $t('username') }}</label>
        </FloatLabel>
        <FloatLabel variant="on">
            <Password v-model="form.username" :feedback="false" fluid  class="ltr"/>
            <label>{{ $t('password') }}</label>
        </FloatLabel>
        <div class="w-90 flex items-center justify-between">
            <Button :label="$t('back')" severity="secondary" outlined @click="dialog.close()" />
            <Button :label="$t('submit')" type="submit" :loading="pending" />
        </div>
    </form>
</template>
