<script setup>
const { router, toast, t } = inject('services')

const auth = useAuthStore()

const form = reactive({ username: '', password: '' })

const handleClick = async () => {
    const { status, data } = await auth.login(form)

    if (status == 200) {
        router.replace('/')
    } else {
        toast.add({ severity: 'error', summary: t('error'), detail: data.error, life: 3000 })
    }
}
</script>

<template>
    <div class="bg-gray-50 h-screen w-screen flex items-center">
        <form class="w-120 h-96 m-auto flex flex-col gap-8 p-8 rounded-2xl border border-gray-100 bg-white shadow"
            @submit.prevent="handleClick()">
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
            <Button :label="$t('login')" type="submit" :loading="auth.loading" />
        </form>
    </div>

</template>
