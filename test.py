import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, TensorDataset
import numpy as np

class MultiplicationNN(nn.Module):
    """
    Нейросетевой модуль для приближённого вычисления произведения двух чисел.
    Автоматически использует GPU при наличии, иначе падает на CPU.
    """
    def __init__(self, input_scale: float = 100.0):
        super().__init__()
        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        self.input_scale = input_scale
        self.output_scale = input_scale ** 2
        
        # Переносим сеть на выбранное устройство
        self.network = nn.Sequential(
            nn.Linear(2, 128),
            nn.GELU(),
            nn.Linear(128, 128),
            nn.GELU(),
            nn.Linear(128, 64),
            nn.GELU(),
            nn.Linear(64, 1)
        ).to(self.device)
        
        # Оптимизации для NVIDIA GPU
        if self.device.type == "cuda":
            torch.backends.cudnn.benchmark = True
            torch.set_float32_matmul_precision('high')
            print(f"✅ GPU инициализирована: {torch.cuda.get_device_name(0)}")
        else:
            print("⚠️ CUDA не найдена. Используется CPU.")

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        # Нормализация -> сеть -> денормализация
        x_norm = x / self.input_scale
        y_norm = self.network(x_norm)
        return y_norm * self.output_scale

    def fit(self, n_samples: int = 10000, epochs: int = 2000, 
            batch_size: int = 128, max_val: int = 100, 
            lr: float = 1e-3, verbose: bool = True):
        """Обучение ТОЛЬКО на целых числах. Данные автоматически переносятся на GPU."""
        a = np.random.randint(-max_val, max_val + 1, size=(n_samples, 1)).astype(np.float32)
        b = np.random.randint(-max_val, max_val + 1, size=(n_samples, 1)).astype(np.float32)
        
        X = torch.from_numpy(np.hstack([a, b]))
        y = torch.from_numpy(a * b)
        
        dataset = TensorDataset(X, y)
        # pin_memory=True + non_blocking=True ускоряют CPU -> GPU копирование
        loader = DataLoader(dataset, batch_size=batch_size, shuffle=True, 
                            pin_memory=True, num_workers=2)
        
        optimizer = optim.Adam(self.parameters(), lr=lr)
        criterion = nn.MSELoss()
        scheduler = optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=epochs)
        
        self.train()
        for epoch in range(epochs):
            epoch_loss = 0.0
            for xb, yb in loader:
                # Асинхронный перенос на GPU
                xb, yb = xb.to(self.device, non_blocking=True), yb.to(self.device, non_blocking=True)
                
                pred = self(xb)
                loss = criterion(pred, yb)
                
                optimizer.zero_grad()
                loss.backward()
                optimizer.step()
                epoch_loss += loss.item()
            scheduler.step()
            
            
            avg_loss = epoch_loss / len(loader)
            print(f"Epoch {epoch+1:4d}/{epochs} | MSE Loss: {avg_loss:.6f}")

    @torch.no_grad()
    def predict(self, x1, x2):
        """Предсказание. Возвращает тензор на CPU для удобства."""
        self.eval()
        if not torch.is_tensor(x1):
            x1 = torch.tensor(x1, dtype=torch.float32).view(-1, 1)
            x2 = torch.tensor(x2, dtype=torch.float32).view(-1, 1)
            
        inputs = torch.cat([x1, x2], dim=1).to(self.device)
        pred = self(inputs)
        return pred.squeeze(-1).cpu()


# ================= ДЕМО-ЗАПУСК =================
if __name__ == "__main__":
    model = MultiplicationNN(input_scale=100.0)
    print("🔄 Обучение на целых числах из диапазона [-100, 100]...")
    model.fit(epochs=500)
    
    print("\n📊 Тестирование на дробных числах:")
    test_cases = [
        (2.5, 3.2), (-4.7, 8.1), (0.5, 0.5), 
        (99.9, -10.0), (17, 13), (150.0, -3.14)
    ]
    
    for a, b in test_cases:
        pred = model.predict(a, b)
        true_val = a * b
        error = abs(pred.item() - true_val)
        rel_error = (error / abs(true_val) * 100) if true_val != 0 else error
        print(f"{a:6.2f} * {b:6.2f} = {pred.item():8.3f} | "
              f"Точно: {true_val:8.3f} | Отн. погрешность: {rel_error:.2f}%")
        
    while True: 
        pred = model.predict(float(input()), float(input()))
        print(pred)
    