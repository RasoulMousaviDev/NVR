<script setup>
definePageMeta({
    layout: false
})
const { fetch: refreshSession } = useUserSession()

const toast = useToast()

const form = reactive({
    username: '',
    password: '',
})

const { pending, execute } = useFetch('/api/login', {
    method: 'POST', immediate: false, body: form,
    async onResponse({ response }) {
        if (response.ok) {
            await refreshSession()
            await navigateTo('/')
        } else {
            toast.add({ severity: 'error', summary: 'Error', detail: 'Bad credentials', life: 3000 })
        }
    }
})


</script>

<template>
    <div class="bg-gray-50 h-screen w-screen flex items-center">
        <form class="w-120 h-96 m-auto flex flex-col gap-8 p-8 rounded-2xl border border-gray-100 bg-white shadow"
            @submit.prevent="execute()">
            <span class="pi pi-user mx-auto text-8xl text-gray-500"></span>
            <FloatLabel variant="on">
                <InputText v-model="form.username" fluid class="ltr" />
                <label>{{ $t('username') }}</label>
            </FloatLabel>
            <FloatLabel variant="on">
                <Password v-model="form.password" :feedback="false" fluid class="ltr" toggle-mask
                    :class="{ '[&>svg]:hidden!': !form.password }" />
                <label>{{ $t('password') }}</label>
            </FloatLabel>
            <Button :label="$t('login')" type="submit" :loading="pending" />
        </form>
    </div>

</template>
