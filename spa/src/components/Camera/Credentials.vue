<script setup>
const { toast } = inject('services')

const dialog = inject('dialogRef')

const store = useCameraStore()

const data = dialog.value.data

const form = reactive({ username: 'admin', password: 'admin' })

const readonly = ref(true)

const handleClick = async () => {

    const status = await store.credentials(data.id, form)

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
            <Button :label="$t('submit')" type="submit" :loading="store.loading" />
        </div>
    </form>
</template>
