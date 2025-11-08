export default defineNuxtRouteMiddleware(() => {
    console.log('defineNuxtRouteMiddleware');
    
    const { loggedIn } = useUserSession();

    if (!loggedIn.value) {
        return navigateTo("/login");
    }
});
